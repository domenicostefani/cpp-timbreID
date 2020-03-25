/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class DemoEditor  : public AudioProcessorEditor, public Button::Listener
{
public:
    DemoEditor (DemoProcessor&);
    ~DemoEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void buttonClicked (Button *);

    Colour bgColor;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DemoProcessor& processor;

    TextButton button;
    Label label;

    std::string sampleRate_s = "";
    std::string blockSize_s = "";
    std::string crossings_s = "";
    void monitorSetSampleRate(double sampleRate);
    void monitorSetBlockSize(int blockSize);
    void monitorSetCrossings(uint32 crossings);
};
