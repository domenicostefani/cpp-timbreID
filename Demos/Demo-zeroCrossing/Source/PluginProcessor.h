/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - zeroCrossing Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"

//==============================================================================
/**
*/
class DemoProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    DemoProcessor();
    ~DemoProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    uint32_t computeZeroCrossing();
    uint32_t getWindowSize();
private:
    unsigned long int WINDOW_SIZE = 1024;
    tid::ZeroCrossing<float> zeroCrossing{WINDOW_SIZE};

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};
