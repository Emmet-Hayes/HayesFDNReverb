#pragma once
#include <JuceHeader.h>

#include "HayesFDNReverbAudioProcessor.h"
#include "DbSlider.h"
#include "LogMsSlider.h"

class HayesFDNReverbAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    HayesFDNReverbAudioProcessorEditor (HayesFDNReverbAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    HayesFDNReverbAudioProcessor& audioProcessor;
    
    juce::Image image;

    LogMsSlider mTimeSliders[DELAY_LINE_COUNT];
    DbSlider mFeedbackSliders[DELAY_LINE_COUNT];
    DbSlider mGainSliders[DELAY_LINE_COUNT];

    juce::Label mGainLabel;
    juce::Label mTimeLabel;
    juce::Label mFeedbackLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mGainAttachment[DELAY_LINE_COUNT];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mTimeAttachment[DELAY_LINE_COUNT];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mFeedbackAttachment[DELAY_LINE_COUNT];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HayesFDNReverbAudioProcessorEditor)
};
