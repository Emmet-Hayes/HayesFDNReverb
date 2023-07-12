#include "CustomLookAndFeel.h"

CustomLookAndFeel::CustomLookAndFeel() 
{
    setColour(juce::Slider::thumbColourId, juce::Colours::darkmagenta);
    setColour(juce::ComboBox::textColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxTextColourId, juce::Colours::white);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::white);
}

void CustomLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height, float sliderPos, 
                                         const float rotaryStartAngle, const float rotaryEndAngle, juce::Slider& slider) 
{
    double radius  = juce::jmin(width / 2, height / 2) - 4.0f;
    double centreX = x + width * 0.5f;
    double centreY = y + height * 0.5f;
    double rx = centreX - radius;
    double ry = centreY - radius;
    double rw = radius * 2.0f;
    double angle = static_cast<double>(rotaryStartAngle) + sliderPos * static_cast<double>(rotaryEndAngle - rotaryStartAngle);
    
    g.setColour(juce::Colours::whitesmoke);
    g.setOpacity(0.33f);
    g.fillEllipse(rx, ry, rw, rw);
  
    g.setColour(juce::Colours::bisque);
    g.setOpacity(1.0f);
    g.drawEllipse(rx, ry, rw, rw, 2.0f);
  
    juce::Path p;
    double pointerLength = radius * 0.5f;
    double pointerThickness = 5.0f;
    p.addRectangle(-pointerThickness * 0.5f, -radius, pointerThickness, pointerLength);
    p.applyTransform(juce::AffineTransform::rotation(angle).translated(centreX, centreY)); //animate
    g.setColour(juce::Colours::seashell);
    g.fillPath(p);
}

void CustomLookAndFeel::drawComboBox(juce::Graphics& g, int w, int h, bool isDown, int bx, int by, 
                              int bw, int bh, juce::ComboBox& cb) 
{
    juce::Colour c(30, 8, 33);
    g.setColour(c);
    g.fillAll();
}

juce::Font CustomLookAndFeel::getComboBoxFont(juce::ComboBox & c) 
{
    return getCommonMenuFont(scale);
}

juce::Font CustomLookAndFeel::getPopupMenuFont() 
{
    return getCommonMenuFont(scale);
}

juce::Font CustomLookAndFeel::getCommonMenuFont(float s) 
{
    return juce::Font("Lucida Console", 10.f * s, juce::Font::bold);
}

void CustomLookAndFeel::drawPopupMenuBackground(juce::Graphics& g, int width, int height) 
{
    juce::Colour c(30, 8, 33);
    g.setColour(c);
    g.fillAll();
}