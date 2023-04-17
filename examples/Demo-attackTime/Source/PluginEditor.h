/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - attackTime Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "SetParamBox.h"

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

    TextButton computeButton;
    Label dataLabel;
    Label resLabel;

    double mSampleRate = 0.0;
    int mBlockSize = 0;

    unsigned long int mPeakSampIdx = 0;
    unsigned long int mAttackStartIdx = 0;
    float mAttackTime = 0.0f;

    SetParamBox setSearchRangeBox;
    SetParamBox setSampMagThreshBox;
    SetParamBox setNumSampsThreshBox;

    void updateDataLabel();
    void updateResLabel();
};
