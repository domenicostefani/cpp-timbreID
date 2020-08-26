/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "juce_timbreID.h"

//==============================================================================
/**
*/
class DemoaubioonsetAudioProcessor  : public AudioProcessor,
                                      private tid::aubio::Onset<float>::Listener
{
public:
    //==============================================================================
    DemoaubioonsetAudioProcessor();
    ~DemoaubioonsetAudioProcessor();

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

    int graphicsCounter = 0;
private:

    /**    Set initial parameters      **/
    const unsigned int WINDOW_SIZE = 2048;
    const unsigned int HOP = 128;
    const float ONSET_THRESHOLD = 0.0f;
    const float ONSET_MINIOI = 0.0f;
    const float SILENCE_THRESHOLD = -90.0f;
    const tid::aubio::OnsetMethod ONSET_METHOD = tid::aubio::OnsetMethod::defaultMethod;

    tid::aubio::Onset<float> aubioOnset{WINDOW_SIZE,HOP,ONSET_THRESHOLD,ONSET_MINIOI,SILENCE_THRESHOLD,ONSET_METHOD};

    void onsetDetected (tid::aubio::Onset<float> *);


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoaubioonsetAudioProcessor)
};
