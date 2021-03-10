/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - bark Module

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
class DemoProcessor : public AudioProcessor,
                      public tid::Bark<float>::Listener
{
public:
    //=========================== Juce System Stuff ============================
    DemoProcessor();
    ~DemoProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //===================== Bark module initialization =============================

    /**    Set initial parameters      **/
    const unsigned int WINDOW_SIZE = 2048;
    const unsigned int HOP = 128;
    const float BARK_SPACING = 0.5;

    /**    Initialize the module       **/
    tid::Bark<float> bark{WINDOW_SIZE, HOP, BARK_SPACING};

    /**    This atomic is used to update the onset LED in the GUI **/
    std::atomic<bool> onsetMonitorState{false};

private:
    void onsetDetected (tid::Bark<float>* bark);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};
