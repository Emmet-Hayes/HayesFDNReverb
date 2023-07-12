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
    numDelayLinesLabel.setText("Number of Delay Lines", juce::NotificationType::dontSendNotification);
    timeLabel.setText("Time", juce::NotificationType::dontSendNotification);
    feedbackLabel.setText("Feedback", juce::NotificationType::dontSendNotification);
    mixLabel.setText("Mix", juce::NotificationType::dontSendNotification);
    numDelayLinesLabel.setJustificationType(juce::Justification::centred);
    timeLabel.setJustificationType(juce::Justification::centred);
    feedbackLabel.setJustificationType(juce::Justification::centred);
    mixLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(numDelayLinesLabel);
    addAndMakeVisible(timeLabel);
    addAndMakeVisible(feedbackLabel);
    addAndMakeVisible(mixLabel);

    numDelayLinesBox.addItemList({ "1", "2", "3", "4", "5", "6", "7", "8" }, 1);
    numDelayLinesBox.setJustificationType(juce::Justification::centred);
    numDelayLinesBox.setLookAndFeel(&customLookAndFeel);
    numDelayLinesBox.addListener(this);
    addAndMakeVisible(numDelayLinesBox);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        timeSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        feedbackSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        mixSliders[i].setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        timeSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        feedbackSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        mixSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        timeSliders[i].setLookAndFeel(&customLookAndFeel);
        feedbackSliders[i].setLookAndFeel(&customLookAndFeel);
        mixSliders[i].setLookAndFeel(&customLookAndFeel);
        addAndMakeVisible(timeSliders[i]);
        addAndMakeVisible(feedbackSliders[i]);
        addAndMakeVisible(mixSliders[i]);
    }

    image = juce::ImageCache::getFromMemory(BinaryData::bg_file_jpg, BinaryData::bg_file_jpgSize);
    setSize(400, 860);

    numDelayLinesAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "numdelaylines", numDelayLinesBox);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
        timeAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "time" + std::to_string(i), timeSliders[i]);
        feedbackAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "feedback" + std::to_string(i), feedbackSliders[i]);
        mixAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "mix" + std::to_string(i), mixSliders[i]);
    }
}

void HayesFDNReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.drawImage(image, 0, 0, getWidth(), getHeight(), 1248, 0, 800, 1600);
}

void HayesFDNReverbAudioProcessorEditor::resized()
{
    numDelayLinesLabel.setBounds(25, 5, 200, 25);
    numDelayLinesBox.setBounds(235, 5, 70, 30);
    timeLabel.setBounds(25, 40, 70, 20);
    feedbackLabel.setBounds(165, 40, 70, 20);
    mixLabel.setBounds(305, 40, 70, 20);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        int y = i * 100 + 65;
        timeSliders[i].setBounds(20, y, 80, 80);
        feedbackSliders[i].setBounds(160, y, 80, 80);
        mixSliders[i].setBounds(300, y, 80, 80);
    }
}

void HayesFDNReverbAudioProcessorEditor::comboBoxChanged(juce::ComboBox* comboBoxThatChanged)
{
    if (comboBoxThatChanged == &numDelayLinesBox)
    {
        int newNumDelayLines = std::stoi(numDelayLinesBox.getText().toStdString());
        for (int i = 0; i < newNumDelayLines; ++i)
        {
            timeSliders[i].setEnabled(true);
            feedbackSliders[i].setEnabled(true);
            mixSliders[i].setEnabled(true);
        }
        for (int i = newNumDelayLines; i < DELAY_LINE_COUNT; ++i)
        {
            timeSliders[i].setEnabled(false);
            feedbackSliders[i].setEnabled(false);
            mixSliders[i].setEnabled(false);
        }
    }
}
