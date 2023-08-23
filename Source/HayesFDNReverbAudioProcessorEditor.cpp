#include "HayesFDNReverbAudioProcessor.h"
#include "HayesFDNReverbAudioProcessorEditor.h"

HayesFDNReverbAudioProcessorEditor::HayesFDNReverbAudioProcessorEditor(HayesFDNReverbAudioProcessor& p)
:   AudioProcessorEditor { &p }
,   audioProcessor { p }
,   presetBar { p }
{
    addAllGUIComponents();
}

HayesFDNReverbAudioProcessorEditor::~HayesFDNReverbAudioProcessorEditor()
{
    setLookAndFeel(nullptr);
}

void HayesFDNReverbAudioProcessorEditor::addAllGUIComponents()
{
    setLookAndFeel(&customLookAndFeel);
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
    numDelayLinesBox.addListener(this);
    addAndMakeVisible(numDelayLinesBox);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        timeSliders[i].setSliderStyle(juce::Slider::Rotary);
        feedbackSliders[i].setSliderStyle(juce::Slider::Rotary);
        mixSliders[i].setSliderStyle(juce::Slider::Rotary);
        timeSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        feedbackSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        mixSliders[i].setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 20);
        addAndMakeVisible(timeSliders[i]);
        addAndMakeVisible(feedbackSliders[i]);
        addAndMakeVisible(mixSliders[i]);
    }

    addAndMakeVisible(presetBar);

    image = juce::ImageCache::getFromMemory(BinaryData::bg_file_jpg, BinaryData::bg_file_jpgSize);

    numDelayLinesAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(audioProcessor.apvts, "numdelaylines", numDelayLinesBox);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        using Attachment = juce::AudioProcessorValueTreeState::SliderAttachment;
        timeAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "time" + std::to_string(i), timeSliders[i]);
        feedbackAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "feedback" + std::to_string(i), feedbackSliders[i]);
        mixAttachment[i] = std::make_unique<Attachment>(audioProcessor.apvts, "mix" + std::to_string(i), mixSliders[i]);
    }

    const auto ratio = static_cast<double> (defaultWidth) / defaultHeight;
    setResizable(false, true);
    getConstrainer()->setFixedAspectRatio(ratio);
    getConstrainer()->setSizeLimits(defaultWidth, defaultHeight, defaultWidth * 2, defaultHeight * 2);
    setSize(defaultWidth, defaultHeight);
}

void HayesFDNReverbAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.drawImage(image, 0, 0, getWidth(), getHeight(), 0, 0, 800, 1720);
}

void HayesFDNReverbAudioProcessorEditor::resized()
{
    const auto scale = static_cast<float> (getWidth()) / defaultWidth;

    auto setBoundsAndApplyScaling = [&](juce::Component* component, int x, int y, int w, int h, bool isSlider = false)
    {
        component->setBounds(static_cast<int>(x * scale), static_cast<int>(y * scale),
                             static_cast<int>(w * scale), static_cast<int>(h * scale));
        if (isSlider)
            dynamic_cast<juce::Slider*>(component)->setTextBoxStyle(juce::Slider::TextBoxBelow, false, static_cast<int>(70 * scale), static_cast<int>(20 * scale));
    };
    
    customLookAndFeel.setWindowScale(scale);
    setBoundsAndApplyScaling(&presetBar, 0, 5, 220, 25);
    setBoundsAndApplyScaling(&numDelayLinesLabel, 220, 5, 125, 25);
    setBoundsAndApplyScaling(&numDelayLinesBox, 345, 5, 50, 30);
    setBoundsAndApplyScaling(&timeLabel, 25, 40, 70, 20);
    setBoundsAndApplyScaling(&feedbackLabel, 165, 40, 70, 20);
    setBoundsAndApplyScaling(&mixLabel, 305, 40, 70, 20);

    for (int i = 0; i < DELAY_LINE_COUNT; ++i)
    {
        int y = i * 100 + 65;
        setBoundsAndApplyScaling(&timeSliders[i], 20, y, 80, 80, true);
        setBoundsAndApplyScaling(&feedbackSliders[i], 160, y, 80, 80, true);
        setBoundsAndApplyScaling(&mixSliders[i], 300, y, 80, 80, true);
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
