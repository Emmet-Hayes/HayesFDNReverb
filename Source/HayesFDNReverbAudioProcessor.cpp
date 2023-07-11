#include "HayesFDNReverbAudioProcessor.h"
#include "HayesFDNReverbAudioProcessorEditor.h"
#include "Utilities.h"

HayesFDNReverbAudioProcessor::HayesFDNReverbAudioProcessor()
:   apvts { *this, nullptr, "HayesDelayParameters", createParameterLayout() }
{
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
}

void HayesFDNReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::AudioBuffer<float> dryBuffer;
    dryBuffer.makeCopyOf(buffer, true);
    
    if (Bus* inputBus = getBus(true, 0))
    {
        // Diffusion
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        for (int j = 0; j < DIFFUSER_COUNT; ++j)
        {
            *allPassFilters[j].state = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(currentSampleRate, 500.0f);
            allPassFilters[j].process(context); 
        }

        //Delay lines
        for (int j = 0; j < DELAY_LINE_COUNT; ++j)
        {
            const float gain = juce::Decibels::decibelsToGain(wetMix[j].get());
            const float time = times[j].get();
            const float feedback = juce::Decibels::decibelsToGain(feedbacks[j].get());

            for (int i = 0; i < delayBuffers[j].getNumChannels(); ++i) // write original to delay
            {
                const int inputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer(std::min(i, inputBus->getNumberOfChannels()));
                writeToDelayBuffer(buffer, j, inputChannelNum, i, writePos[j], 1.0f, 1.0f, true);
            }

            auto readPos = juce::roundToInt(writePos[j] - (currentSampleRate * time / 1000.0)); // read delayed signal
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
                
            for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            {
                buffer.applyGain(channel, 0, buffer.getNumSamples(), wetMix[j].get());
                buffer.addFromWithRamp(channel, 0, dryBuffer.getReadPointer(channel), dryBuffer.getNumSamples(), 1.0f - wetMix[j].get(), 1.0f - wetMix[j].get());
            }
        }
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

void HayesFDNReverbAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::MemoryOutputStream mos(destData, true);
    apvts.state.writeToStream(mos);
}

void HayesFDNReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data,
        static_cast<size_t> (sizeInBytes));

    if (tree.isValid())
        apvts.replaceState(tree);
}

void HayesFDNReverbAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
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
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { gstr, 1 }, "mix", 0.0f, 1.0f, 0.1f));

    }
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HayesFDNReverbAudioProcessor();
}
