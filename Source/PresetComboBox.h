#pragma once
#include <JuceHeader.h>
#include "HayesFDNReverbAudioProcessor.h"

class PresetBar;

class PresetComboBox : public juce::ComboBox
{
public:
    PresetComboBox(HayesFDNReverbAudioProcessor& p, PresetBar* bar);
    void mouseDown(const juce::MouseEvent& event) override;
    void savePreset();
    void loadPreset();
    void deletePreset();

    HayesFDNReverbAudioProcessor& processor;
    PresetBar* presetBar;
    juce::PopupMenu saveLoadMenu;
};
