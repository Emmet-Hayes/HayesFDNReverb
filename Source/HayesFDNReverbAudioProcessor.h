#pragma once
#include <JuceHeader.h>

inline static const double TAIL_LENGTH { 2.0 };
inline static const int DELAY_LINE_COUNT { 8 };
inline static const int DIFFUSER_COUNT { 9 };

class HayesFDNReverbAudioProcessor : public juce::AudioProcessor
                                   , public juce::AudioProcessorValueTreeState::Listener
{
public:
    HayesFDNReverbAudioProcessor();

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }

    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return JucePlugin_Name; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState mState;

private:
    void writeToDelayBuffer(juce::AudioSampleBuffer& buffer, int delayLineIndex,
        const int channelIn, const int channelOut,
        const int writePos,
        float startGain, float endGain,
        bool replacing);

    void readFromDelayBuffer(juce::AudioSampleBuffer& buffer, int delayLineIndex,
        const int channelIn, const int channelOut,
        const int readPos,
        float startGain, float endGain,
        bool replacing);

    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::dsp::ProcessorDuplicator<juce::dsp::IIR::Filter<float>, juce::dsp::IIR::Coefficients<float>> allPassFilters[DIFFUSER_COUNT];
    juce::Atomic<float>     mGains[DELAY_LINE_COUNT] { -18.0f };
    juce::Atomic<float>     mTimes[DELAY_LINE_COUNT] { 200.0f };
    juce::Atomic<float>     mFeedbacks[DELAY_LINE_COUNT] { -18.0f };
    juce::AudioSampleBuffer mDelayBuffers[DELAY_LINE_COUNT];
    float                   mLastInputGain[DELAY_LINE_COUNT] { 0.0f };
    float                   mLastFeedbackGain[DELAY_LINE_COUNT] { 0.0f };
    int                     mWritePos[DELAY_LINE_COUNT] { 0 };
    int                     mExpectedReadPos[DELAY_LINE_COUNT] { -1 };
    double                  mSampleRate { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HayesFDNReverbAudioProcessor)
};
