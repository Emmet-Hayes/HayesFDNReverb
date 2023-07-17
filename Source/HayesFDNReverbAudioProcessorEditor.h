#pragma once
#include <JuceHeader.h>
#include "HayesFDNReverbAudioProcessor.h"
#include "CustomLookAndFeel.h"
#include "DbSlider.h"
#include "LogMsSlider.h"
#include "PercentSlider.h"
#include "PresetBar.h"

class HayesFDNReverbAudioProcessorEditor  : public juce::AudioProcessorEditor
                                          , public juce::ComboBox::Listener
{
public:
    HayesFDNReverbAudioProcessorEditor(HayesFDNReverbAudioProcessor&);
    void paint(juce::Graphics&) override;
    void resized() override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatChanged) override;

private:
    void addAllGUIComponents();

    HayesFDNReverbAudioProcessor& audioProcessor;
    
    CustomLookAndFeel customLookAndFeel;
    
    PresetBar presetBar;
    juce::Image image;

    juce::ComboBox numDelayLinesBox;
    LogMsSlider timeSliders[DELAY_LINE_COUNT];
    DbSlider feedbackSliders[DELAY_LINE_COUNT];
    PercentSlider mixSliders[DELAY_LINE_COUNT];

    juce::Label numDelayLinesLabel;
    juce::Label timeLabel;
    juce::Label feedbackLabel;
    juce::Label mixLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> numDelayLinesAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment[DELAY_LINE_COUNT];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment[DELAY_LINE_COUNT];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment[DELAY_LINE_COUNT];

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HayesFDNReverbAudioProcessorEditor)
};
