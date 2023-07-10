#include "HayesFDNReverbAudioProcessor.h"
#include "HayesFDNReverbAudioProcessorEditor.h"
#include "Utilities.h"

HayesFDNReverbAudioProcessor::HayesFDNReverbAudioProcessor()
:   mState { *this, nullptr, "HayesDelayParameters", createParameterLayout() }
{

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        mState.addParameterListener("time" + std::to_string(i), this);
        mState.addParameterListener("feedback" + std::to_string(i), this);
        mState.addParameterListener("gain" + std::to_string(i), this);
    }
}

void HayesFDNReverbAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    mSampleRate = sampleRate;
    juce::dsp::ProcessSpec spec{ sampleRate,
                              static_cast<juce::uint32>(samplesPerBlock),
                              static_cast<juce::uint32>(getTotalNumOutputChannels()) };

    for (int i = 0; i < DIFFUSER_COUNT; ++i)
        allPassFilters[i].prepare(spec);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        mDelayBuffers[i].setSize(getTotalNumOutputChannels(), TAIL_LENGTH * (samplesPerBlock + sampleRate), false, false);
        mDelayBuffers[i].clear();
        mExpectedReadPos[i] = -1;
    }


}

void HayesFDNReverbAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    if (Bus* inputBus = getBus(true, 0))
    {
        // Diffusion
        juce::dsp::AudioBlock<float> block(buffer);
        juce::dsp::ProcessContextReplacing<float> context(block);
        for (int j = 0; j < DIFFUSER_COUNT; ++j)
        {
            *allPassFilters[j].state = juce::dsp::IIR::ArrayCoefficients<float>::makeAllPass(mSampleRate, 500.0f);
            allPassFilters[j].process(context); 
        }

        //Delay lines
        for (int j = 0; j < DELAY_LINE_COUNT; ++j)
        {
            const float gain = juce::Decibels::decibelsToGain(mGains[j].get());
            const float time = mTimes[j].get();
            const float feedback = juce::Decibels::decibelsToGain(mFeedbacks[j].get());


            for (int i = 0; i < mDelayBuffers[j].getNumChannels(); ++i) // write original to delay
            {
                const int inputChannelNum = inputBus->getChannelIndexInProcessBlockBuffer(std::min(i, inputBus->getNumberOfChannels()));
                writeToDelayBuffer(buffer, j, inputChannelNum, i, mWritePos[j], 1.0f, 1.0f, true);
            }

            buffer.applyGainRamp(0, buffer.getNumSamples(), mLastInputGain[j], gain); // adapt dry gain
            mLastInputGain[j] = gain;

            auto readPos = juce::roundToInt(mWritePos[j] - (mSampleRate * time / 1000.0)); // read delayed signal
            if (readPos < 0)
                readPos += mDelayBuffers[j].getNumSamples();

            if (Bus* outputBus = getBus(false, 0))
            {
                if (mExpectedReadPos[j] >= 0) // if has run before
                {
                    auto endGain = (readPos == mExpectedReadPos[j]) ? 1.0f : 0.0f; // fade out if readPos is off
                    for (int i = 0; i < outputBus->getNumberOfChannels(); ++i)
                    {
                        const int outputChannelNum = outputBus->getChannelIndexInProcessBlockBuffer(i);
                        readFromDelayBuffer(buffer, j, i, outputChannelNum, mExpectedReadPos[j], 1.0, endGain, false);
                    }
                }

                if (readPos != mExpectedReadPos[j]) // fade in at new position
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
                writeToDelayBuffer(buffer, j, outputChannelNum, i, mWritePos[j], mLastFeedbackGain[j], feedback, false);
            }
            mLastFeedbackGain[j] = feedback;

            mWritePos[j] += buffer.getNumSamples(); // advance positions
            if (mWritePos[j] >= mDelayBuffers[j].getNumSamples())
                mWritePos[j] -= mDelayBuffers[j].getNumSamples();

            mExpectedReadPos[j] = readPos + buffer.getNumSamples();
            if (mExpectedReadPos[j] >= mDelayBuffers[j].getNumSamples())
                mExpectedReadPos[j] -= mDelayBuffers[j].getNumSamples();
        }
    }
}

void HayesFDNReverbAudioProcessor::writeToDelayBuffer(juce::AudioSampleBuffer& buffer, int delayLineIndex,
    const int channelIn, const int channelOut,
    const int writePos, float startGain, float endGain, bool replacing)
{
    if (writePos + buffer.getNumSamples() <= mDelayBuffers[delayLineIndex].getNumSamples())
    {
        if (replacing)
            mDelayBuffers[delayLineIndex].copyFromWithRamp(channelOut, writePos, buffer.getReadPointer(channelIn), buffer.getNumSamples(), startGain, endGain);
        else
            mDelayBuffers[delayLineIndex].addFromWithRamp(channelOut, writePos, buffer.getReadPointer(channelIn), buffer.getNumSamples(), startGain, endGain);
    }
    else
    {
        const auto midPos = mDelayBuffers[delayLineIndex].getNumSamples() - writePos;
        const auto midGain = juce::jmap(float(midPos) / buffer.getNumSamples(), startGain, endGain);
        if (replacing)
        {
            mDelayBuffers[delayLineIndex].copyFromWithRamp(channelOut, writePos, buffer.getReadPointer(channelIn), midPos, startGain, midGain);
            mDelayBuffers[delayLineIndex].copyFromWithRamp(channelOut, 0, buffer.getReadPointer(channelIn, midPos), buffer.getNumSamples() - midPos, midGain, endGain);
        }
        else
        {
            mDelayBuffers[delayLineIndex].addFromWithRamp(channelOut, writePos, buffer.getReadPointer(channelIn), midPos, mLastInputGain[delayLineIndex], midGain);
            mDelayBuffers[delayLineIndex].addFromWithRamp(channelOut, 0, buffer.getReadPointer(channelIn, midPos), buffer.getNumSamples() - midPos, midGain, endGain);
        }
    }
}

void HayesFDNReverbAudioProcessor::readFromDelayBuffer(juce::AudioSampleBuffer& buffer, int delayLineIndex,
    const int channelIn, const int channelOut,
    const int readPos,
    float startGain, float endGain,
    bool replacing)
{
    if (readPos + buffer.getNumSamples() <= mDelayBuffers[delayLineIndex].getNumSamples())
    {
        if (replacing)
            buffer.copyFromWithRamp(channelOut, 0, mDelayBuffers[delayLineIndex].getReadPointer(channelIn, readPos), buffer.getNumSamples(), startGain, endGain);
        else
            buffer.addFromWithRamp(channelOut, 0, mDelayBuffers[delayLineIndex].getReadPointer(channelIn, readPos), buffer.getNumSamples(), startGain, endGain);
    }
    else
    {
        const auto midPos = mDelayBuffers[delayLineIndex].getNumSamples() - readPos;
        const auto midGain = juce::jmap(float(midPos) / buffer.getNumSamples(), startGain, endGain);
        if (replacing)
        {
            buffer.copyFromWithRamp(channelOut, 0, mDelayBuffers[delayLineIndex].getReadPointer(channelIn, readPos), midPos, startGain, midGain);
            buffer.copyFromWithRamp(channelOut, midPos, mDelayBuffers[delayLineIndex].getReadPointer(channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
        }
        else
        {
            buffer.addFromWithRamp(channelOut, 0, mDelayBuffers[delayLineIndex].getReadPointer(channelIn, readPos), midPos, startGain, midGain);
            buffer.addFromWithRamp(channelOut, midPos, mDelayBuffers[delayLineIndex] .getReadPointer(channelIn), buffer.getNumSamples() - midPos, midGain, endGain);
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
    mState.state.writeToStream(mos);
}

void HayesFDNReverbAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    auto tree = juce::ValueTree::readFromData(data,
        static_cast<size_t> (sizeInBytes));

    if (tree.isValid())
        mState.replaceState(tree);
}

void HayesFDNReverbAudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        if (parameterID == juce::String("gain" + std::to_string(i)))
            mGains[i] = newValue;
        else if (parameterID == juce::String("time" + std::to_string(i)))
            mTimes[i] = newValue;
        else if (parameterID == juce::String("feedback" + std::to_string(i)))
            mFeedbacks[i] = newValue;
    }
}

juce::AudioProcessorValueTreeState::ParameterLayout HayesFDNReverbAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        std::string tstr = "time" + std::to_string(i);
        std::string fstr = "feedback" + std::to_string(i);
        std::string gstr = "gain" + std::to_string(i); 

        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { tstr, 1 }, "Time", makeLogarithmicRange(0.0f, 2000.0f), 30.0f * (i + 1)));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { fstr, 1 }, "Feedback", -100.0f, -0.2f, -21.0f));
        layout.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID { gstr, 1 }, "Gain", -100.0f, 3.0f, -18.0f));

    }
    return layout;
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HayesFDNReverbAudioProcessor();
}
