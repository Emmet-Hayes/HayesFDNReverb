#pragma once
#include <JuceHeader.h>
#include "HayesFDNReverbAudioProcessor.h"
#include "../../Common/BaseAudioProcessorEditor.h"
#include "../../Common/CustomLookAndFeel.h"
#include "../../Common/DbSlider.h"
#include "../../Common/LogMsSlider.h"
#include "../../Common/PercentSlider.h"
#include "../../Common/PresetBar.h"

class HayesFDNReverbAudioProcessorEditor  : public juce::AudioProcessorEditor
                                          , public juce::ComboBox::Listener
{
public:
    HayesFDNReverbAudioProcessorEditor(HayesFDNReverbAudioProcessor&);
    ~HayesFDNReverbAudioProcessorEditor();
    void paint(juce::Graphics&) override;
    void resized() override;
    void comboBoxChanged(juce::ComboBox* comboBoxThatChanged) override;

private:
    void addAllGUIComponents();

    CustomLookAndFeel customLookAndFeel;
    
    HayesFDNReverbAudioProcessor& audioProcessor;
    
    PresetBar presetBar;
    juce::Image image;

    juce::ComboBox numDelayLinesBox;
    LogMsSlider timeSliders[DELAY_LINE_COUNT];
    DbSlider feedbackSliders[DELAY_LINE_COUNT];
    PercentSlider mixSliders[DELAY_LINE_COUNT];

    juce::Label numDelayLinesLabel, timeLabel, feedbackLabel, mixLabel;

    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> numDelayLinesAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> timeAttachment[DELAY_LINE_COUNT];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> feedbackAttachment[DELAY_LINE_COUNT];
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> mixAttachment[DELAY_LINE_COUNT];

    int defaultWidth = 400;
    int defaultHeight = 860;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HayesFDNReverbAudioProcessorEditor)
};
