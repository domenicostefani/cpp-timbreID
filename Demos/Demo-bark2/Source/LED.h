/*
  ==============================================================================

    LED.h
    Created: 8 Apr 2020 3:51:15pm
    Author:  domenico

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class LED    : public Component, private Timer
{
public:
    LED(){}
    ~LED(){}

    void paint (Graphics& g) override
    {
        g.fillAll ((ledState)?Colours::orange:Colours::grey);   // clear the background

        g.setColour (Colours::black);
        g.drawRect (getLocalBounds(), 1);   // draw an outline around the component

        g.setColour (Colours::white);
        g.setFont (14.0f);
        g.drawText ("Onset", getLocalBounds(),
                    Justification::centred, true);   // draw some placeholder text
    }

    void switchOn()
    {
        Timer::startTimer(30);
        ledState = true;
        this->repaint();
    }

    void resized() override
    {
    }

private:
    bool ledState = false;

    void timerCallback() override //Todo: try to move to private and change inheritance
    {
        Timer::stopTimer();
        ledState = false;
        this->repaint();
    }

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (LED)
};
