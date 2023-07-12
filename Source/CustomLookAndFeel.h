#pragma once
#include <JuceHeader.h>

class CustomLookAndFeel : public juce::LookAndFeel_V4
{
public:
	CustomLookAndFeel();
	void drawRotarySlider(juce::Graphics&, int, int, int, int, float, const float,
		const float, juce::Slider&) override;
	void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
	juce::Font getComboBoxFont(juce::ComboBox&) override;
	juce::Font getPopupMenuFont() override;
	void drawPopupMenuBackground(juce::Graphics&, int, int) override;
	inline void setScale(const float s) { scale = s; }

private:
	juce::Font getCommonMenuFont(float);

	float scale = 1.0f;
	float rotaryOutlineBrightness = 1.0f;

};
