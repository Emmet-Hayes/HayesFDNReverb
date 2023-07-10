#include "HayesFDNReverbAudioProcessor.h"
#include "HayesFDNReverbAudioProcessorEditor.h"

HayesFDNReverbAudioProcessorEditor::HayesFDNReverbAudioProcessorEditor(HayesFDNReverbAudioProcessor& p)
:   AudioProcessorEditor { &p }
,   audioProcessor { p }
{
    mTimeLabel.setText("Time", juce::NotificationType::dontSendNotification);
    mFeedbackLabel.setText("Feedback", juce::NotificationType::dontSendNotification);
    mGainLabel.setText("Gain", juce::NotificationType::dontSendNotification);

    mTimeLabel.setJustificationType(juce::Justification::centred);
    mFeedbackLabel.setJustificationType(juce::Justification::centred);
    mGainLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(mTimeLabel);
    addAndMakeVisible(mFeedbackLabel);
    addAndMakeVisible(mGainLabel);

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    
    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        mTimeSliders[i].setSliderStyle(      juce::Slider::RotaryHorizontalVerticalDrag);
        mFeedbackSliders[i].setSliderStyle(  juce::Slider::RotaryHorizontalVerticalDrag);
        mGainSliders[i].setSliderStyle(      juce::Slider::RotaryHorizontalVerticalDrag);
        mTimeSliders[i].setTextBoxStyle(     juce::Slider::TextBoxBelow, false, 70, 20);
        mFeedbackSliders[i].setTextBoxStyle( juce::Slider::TextBoxBelow, false, 70, 20);
        mGainSliders[i].setTextBoxStyle(     juce::Slider::TextBoxBelow, false, 70, 20);
        
        addAndMakeVisible(mTimeSliders[i]);
        addAndMakeVisible(mFeedbackSliders[i]);
        addAndMakeVisible(mGainSliders[i]);
    }

    image = juce::ImageCache::getFromMemory(BinaryData::bg_file_jpg, BinaryData::bg_file_jpgSize);
    setSize(400, 820);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        mTimeAttachment[i] = std::make_unique<Attachment>(audioProcessor.mState, "time" + std::to_string(i), mTimeSliders[i]);
        mFeedbackAttachment[i] = std::make_unique<Attachment>(audioProcessor.mState, "feedback" + std::to_string(i), mFeedbackSliders[i]);
        mGainAttachment[i] = std::make_unique<Attachment>(audioProcessor.mState, "gain" + std::to_string(i), mGainSliders[i]);
    }
}

void HayesFDNReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.drawImage(image, 0, 0, getWidth(), getHeight(), 1248, 0, 800, 1600);
}

void HayesFDNReverbAudioProcessorEditor::resized()
{
    mTimeLabel.setBounds(25, 0, 70, 20);
    mFeedbackLabel.setBounds(165, 0, 70, 20);
    mGainLabel.setBounds(305, 0, 70, 20);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        int y = i * 100 + 25;
        mTimeSliders[i].setBounds(20, y, 80, 80);
        mFeedbackSliders[i].setBounds(160, y, 80, 80);
        mGainSliders[i].setBounds(300, y, 80, 80);
    }
}
