#include "HayesFDNReverbAudioProcessor.h"
#include "HayesFDNReverbAudioProcessorEditor.h"

HayesFDNReverbAudioProcessorEditor::HayesFDNReverbAudioProcessorEditor(HayesFDNReverbAudioProcessor& p)
:   AudioProcessorEditor { &p }
,   audioProcessor { p }
{
    addAllGUIComponents();
}

void HayesFDNReverbAudioProcessorEditor::addAllGUIComponents()
{
    timeLabel.setText("Time", juce::NotificationType::dontSendNotification);
    feedbackLabel.setText("Feedback", juce::NotificationType::dontSendNotification);
    gainLabel.setText("Gain", juce::NotificationType::dontSendNotification);

    timeLabel.setJustificationType(juce::Justification::centred);
    feedbackLabel.setJustificationType(juce::Justification::centred);
    gainLabel.setJustificationType(juce::Justification::centred);

    addAndMakeVisible(timeLabel);
    addAndMakeVisible(feedbackLabel);
    addAndMakeVisible(gainLabel);

    using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        timeSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        feedbackSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        gainSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        timeSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        feedbackSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        gainSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);

        addAndMakeVisible(timeSliders[i]);
        addAndMakeVisible(feedbackSliders[i]);
        addAndMakeVisible(gainSliders[i]);
    }

    image = juce::ImageCache::getFromMemory(BinaryData::bg_file_jpg, BinaryData::bg_file_jpgSize);
    setSize(400, 820);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        timeAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "time" + std::to_string(i), timeSliders[i]);
        feedbackAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "feedback" + std::to_string(i), feedbackSliders[i]);
        gainAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "gain" + std::to_string(i), gainSliders[i]);
    }
}


void HayesFDNReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.drawImage(image, 0, 0, getWidth(), getHeight(), 1248, 0, 800, 1600);
}

void HayesFDNReverbAudioProcessorEditor::resized()
{
    timeLabel.setBounds(25, 0, 70, 20);
    feedbackLabel.setBounds(165, 0, 70, 20);
    gainLabel.setBounds(305, 0, 70, 20);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        int y = i * 100 + 25;
        timeSliders[i].setBounds(20, y, 80, 80);
        feedbackSliders[i].setBounds(160, y, 80, 80);
        gainSliders[i].setBounds(300, y, 80, 80);
    }
}