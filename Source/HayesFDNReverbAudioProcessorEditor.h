#pragma once
#include <JuceHeader.h>

#include "HayesFDNReverbAudioProcessor.h"
#include "DbSlider.h"
#include "LogMsSlider.h"
#include "PercentSlider.h"

class HayesFDNReverbAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    HayesFDNReverbAudioProcessorEditor (HayesFDNReverbAudioProcessor&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void addAllGUIComponents();

    HayesFDNReverbAudioProcessor& audioProcessor;
 
    juce::Image image;

    LogMsSlider timeSliders[DELAY_LINE_COUNT];
    DbSlider feedbackSliders[DELAY_LINE_COUNT];
    PercentSlider mixSliders[DELAY_LINE_COUNT];

    juce::Label timeLabel;
    juce::Label feedbackLabel;
    juce::Label mixLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment[DELAY_LINE_COUNT];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment[DELAY_LINE_COUNT];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment[DELAY_LINE_COUNT];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HayesFDNReverbAudioProcessorEditor)
};
