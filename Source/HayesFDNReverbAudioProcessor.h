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
    double getTailLengthSeconds() const override { return 2.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int index) override {}
    const juce::String getProgramName(int index) override { return JucePlugin_Name; }
    void changeProgramName(int index, const juce::String& newName) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;
    void parameterChanged(const juce::String& parameterID, float newValue) override;

    juce::AudioProcessorValueTreeState apvts;

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
    juce::Atomic<float>     gains[DELAY_LINE_COUNT] { 0.0f, -100.0f, -100.0f, -100.0f, -100.0f, -100.0f, -100.0f, -100.0f };
    juce::Atomic<float>     times[DELAY_LINE_COUNT] { 30.0f, 60.0f, 90.0f, 120.0f, 150.0f, 180.0f, 210.0f, 240.0f };
    juce::Atomic<float>     feedbacks[DELAY_LINE_COUNT] { -18.0f, -18.0f, -18.0f, -18.0f, -18.0f, -18.0f, -18.0f, -18.0f };
    juce::AudioSampleBuffer delayBuffers[DELAY_LINE_COUNT];
    float                   lastInputGain[DELAY_LINE_COUNT] { -100.0f };
    float                   lastFeedbackGain[DELAY_LINE_COUNT] { -18.0f };
    int                     writePos[DELAY_LINE_COUNT] { 0 };
    int                     expectedReadPos[DELAY_LINE_COUNT] { -1 };
    double                  currentSampleRate { 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(HayesFDNReverbAudioProcessor)
};
