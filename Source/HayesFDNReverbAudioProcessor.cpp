#include "../../Common/Utilities.h"

#include "HayesFDNReverbAudioProcessor.h"
#include "HayesFDNReverbAudioProcessorEditor.h"


HayesFDNReverbAudioProcessor::HayesFDNReverbAudioProcessor()
:   BaseAudioProcessor { createParameterLayout() }
{
    apvts.addParameterListener("numdelaylines", this);
    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        apvts.addParameterListener("time" + std::to_string(i), this);
        apvts.addParameterListener("feedback" + std::to_string(i), this);
        apvts.addParameterListener("mix" + std::to_string(i), this);
    }
}

void HayesFDNReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec{ sampleRate,
                              static_cast<juce::uint32>(samplesPerBlock),
                              static_cast<juce::uint32>(getTotalNumOutputChannels()) };

    for (int i = 0; i < DIFFUSER_COUNT; ++i)
        allPassFilters[i].prepare(spec);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        delayBuffers[i].setSize(getTotalNumOutputChannels(), static_cast<int>(TAIL_LENGTH * (samplesPerBlock + sampleRate)), false, false);
        delayBuffers[i].clear();
        expectedReadPos[i] = -1;
    }
    
    capacitor.prepare(spec);
    inductor.prepare(spec);
    otherFilter.prepare(spec);
}

void HayesFDNReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    
    juce::AudioBuffer<float> dryBuffer; // copying the dry buffer to be mixed back in later based on mix knob value
    dryBuffer.makeCopyOf(buffer, true);
    
    if (Bus* inputBus = getBus(true, 0))
    {
        // Diffusion
        for (int j = 0; j < DIFFUSER_COUNT; ++j)
        {
            juce::dsp::AudioBlock<float> block(buffer);
            juce::dsp::ProcessContextReplacing<float> context(block);
            *allPassFilters[j].state = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(currentSampleRate, 500.0f);
            allPassFilters[j].process(context); 
        }

        //Delay lines
        const int activeLines = numActiveDelayLines.get();
        for (int j = 0; j < activeLines; ++j)
        {
            const float time = times[j].get();
            const float feedback = juce::Decibels::decibelsToGain(feedbacks[j].get());

            for (int i = 0; i < delayBuffers[j].getNumChannels(); ++i) // write original to delay
            {
                const int inputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer(std::min(i, inputBus->getNumberOfChannels()));
                writeToDelayBuffer(buffer, j, inputChannelNum, i, writePos[j], 1.0f, 1.0f, true);
            }

            auto readPos =  juce::roundToInt(writePos[j] - (currentSampleRate * time / 1000.0)); // read delayed signal
            if (readPos < 0)
                readPos += delayBuffers[j].getNumSamples();

            if (Bus* outputBus = getBus(false, 0))
            {
                if (expectedReadPos[j] >= 0) // if has run before
                {
                    auto endGain = (readPos == expectedReadPos[j]) ? 1.0f : 0.0f; // fade out if readPos is off
                    for (int i = 0; i < outputBus->getNumberOfChannels(); ++i)
                    {
                        const int outputChannelNum = outputBus->getChannelIndexInProcessBlockBuffer(i);
                        readFromDelayBuffer(buffer, j, i, outputChannelNum, expectedReadPos[j], 1.0, endGain, false);
                    }
                }

                if (readPos != expectedReadPos[j]) // fade in at new position
                {
                    for (int i = 0; i < outputBus->getNumberOfChannels(); ++i)
                    {
                        const int outputChannelNum = outputBus->getChannelIndexInProcessBlockBuffer(i);
                        readFromDelayBuffer(buffer, j, i, outputChannelNum, readPos, 0.0, 1.0, false);
                    }
                }
            }

            for (int i = 0; i < inputBus->getNumberOfChannels(); ++i) // add feedback to delay
            {
                const int outputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer(i);
                writeToDelayBuffer(buffer, j, outputChannelNum, i, writePos[j], lastFeedbackGain[j], feedback, false);
            }
            lastFeedbackGain[j] = feedback;

            writePos[j] += buffer.getNumSamples(); // advance positions
            if (writePos[j] >= delayBuffers[j].getNumSamples())
                writePos[j] -= delayBuffers[j].getNumSamples();

            expectedReadPos[j] = readPos + buffer.getNumSamples();
            if (expectedReadPos[j] >= delayBuffers[j].getNumSamples())
                expectedReadPos[j] -= delayBuffers[j].getNumSamples();
            
            // finally, mix dry (copied before processing) and wet (current buffer)
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel) 
            {
                buffer.applyGain(channel, 0, buffer.getNumSamples(), wetMix[j].get());
                buffer.addFromWithRamp(channel, 0, dryBuffer.getReadPointer(channel), dryBuffer.getNumSamples(), 1.0f - wetMix[j].get(), 1.0f - wetMix[j].get());
            }
        }
        
        // Final touches (IIR filters)
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        
        *capacitor.state = juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass(currentSampleRate, 96.0f, 0.5f);
        capacitor.process(context);
        *inductor.state = juce::dsp::IIR::ArrayCoefficients<float>::makeLowPass(currentSampleRate, 12020.0f, 0.5f);
        inductor.process(context);
    }
}

void HayesFDNReverbAudioProcessor::writeToDelayBuffer(juce::AudioSampleBuffer& buffer, int delayLineIndex,
    const int channelIn, const int channelOut,
    const int writePosition, float startGain, float endGain, bool replacing)
{
    if (writePosition + buffer.getNumSamples() <= delayBuffers[delayLineIndex].getNumSamples())
    {
        if (replacing)
            delayBuffers[delayLineIndex].copyFromWithRamp(channelOut, writePosition, buffer.getReadPointer(channelIn), buffer.getNumSamples(), startGain, endGain);
        else
            delayBuffers[delayLineIndex].addFromWithRamp(channelOut, writePosition, buffer.getReadPointer(channelIn), buffer.getNumSamples(), startGain, endGain);
    }
    else
    {
        const auto midPos = delayBuffers[delayLineIndex].getNumSamples() - writePosition;
        const auto midGain = juce::jmap(float(midPos) / buffer.getNumSamples(), startGain, endGain);
        if (replacing)
        {
            delayBuffers[delayLineIndex].copyFromWithRamp(channelOut, writePosition, buffer.getReadPointer(channelIn), midPos, startGain, midGain);
            delayBuffers[delayLineIndex].copyFromWithRamp(channelOut, 0, buffer.getReadPointer(channelIn, midPos), buffer.getNumSamples() - midPos, midGain, endGain);
        }
        else
        {
            delayBuffers[delayLineIndex].addFromWithRamp(channelOut, writePosition, buffer.getReadPointer(channelIn), midPos, lastInputGain[delayLineIndex], midGain);
            delayBuffers[delayLineIndex].addFromWithRamp(channelOut, 0, buffer.getReadPointer(channelIn, midPos), buffer.getNumSamples() - midPos, midGain, endGain);
        }
    }
}

void HayesFDNReverbAudioProcessor::readFromDelayBuffer(juce::AudioSampleBuffer& buffer, int delayLineIndex,
    const int channelIn, const int channelOut,
    const int readPos,
    float startGain, float endGain,
    bool replacing)
{
    if (readPos + buffer.getNumSamples() <= delayBuffers[delayLineIndex].getNumSamples())
    {
        if (replacing)
            buffer.copyFromWithRamp(channelOut, 0, delayBuffers[delayLineIndex].getReadPointer(channelIn, readPos), buffer.getNumSamples(), startGain, endGain);
        else
            buffer.addFromWithRamp(channelOut, 0, delayBuffers[delayLineIndex].getReadPointer(channelIn, readPos), buffer.getNumSamples(), startGain, endGain);
    }
    else
    {
        const auto midPos = delayBuffers[delayLineIndex].getNumSamples() - readPos;
        const auto midGain = juce::jmap(float(midPos) / buffer.getNumSamples(), startGain, endGain);
        if (replacing)
        {
            buffer.copyFromWithRamp(channelOut, 0, delayBuffers[delayLineIndex].getReadPointer(channelIn, readPos), midPos, startGain, midGain);
            buffer.copyFromWithRamp(channelOut, midPos, delayBuffers[delayLineIndex].getReadPointer(channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
        }
        else
        {
            buffer.addFromWithRamp(channelOut, 0, delayBuffers[delayLineIndex].getReadPointer(channelIn, readPos), midPos, startGain, midGain);
            buffer.addFromWithRamp(channelOut, midPos, delayBuffers[delayLineIndex] .getReadPointer(channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
        }
    }
}

juce::AudioProcessorEditor* HayesFDNReverbAudioProcessor::createEditor()
{
    return new HayesFDNReverbAudioProcessorEditor(*this);
}

void HayesFDNReverbAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{ 
    if (parameterID == juce::String("numdelaylines"))
    {
        numActiveDelayLines = static_cast<int>(std::round(newValue));
        return;
    }

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        if (parameterID == juce::String("time" + std::to_string(i)))
            times[i] = newValue;
        else if (parameterID == juce::String("feedback" + std::to_string(i)))
            feedbacks[i] = newValue;
        else if (parameterID == juce::String("mix" + std::to_string(i)))
            wetMix[i] = newValue;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout HayesFDNReverbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        std::string tstr = "time" + std::to_string(i);
        std::string fstr = "feedback" + std::to_string(i);
        std::string gstr = "mix" + std::to_string(i); 

        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { tstr, 1 }, "Time", makeLogarithmicRange(0.0f, 2000.0f), 30.0f * (i + 1)));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { fstr, 1 }, "Feedback", -100.0f, -1.0f, -25.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { gstr, 1 }, "Mix", 0.0f, 1.0f, 0.10f));
    }
    layout.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{ "numdelaylines", 1 }, "NumDelayLines", 
                                                           juce::StringArray { "1", "2", "3", "4", "5", "6", "7", "8" }, 8));
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HayesFDNReverbAudioProcessor();
}
