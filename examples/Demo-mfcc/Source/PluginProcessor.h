/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - mfcc Module (Mel Frequency Cepstral Coefficients)

  Mel-frequency cepstrum is much different than raw cepstrum.
  The most significant differences are an emphasis on lower spectral content and
  the use of a DCT rather than a FT in the final step of the process. When mfcc~
  receives a bang, it spits out the MFCCs for the most recent analysis window as
  a list. The default 100mel filterbank spacing produces a 38-component MFCC
  vector regardless of window size. MFCC components are normalized to be between
  1 and -1 by default.
  If normalized, the first MFCC will always have a value of 1

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 27th April 2020

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"

//==============================================================================
class DemoProcessor : public AudioProcessor
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

    //===================== module initialization =============================

    /**    Set initial parameters      **/
    const unsigned int WINDOW_SIZE = 1024;
    const float MEL_SPACING = 100;
    /**    Initialize the module       **/
    tid::Mfcc<float> mfcc{this->WINDOW_SIZE, this->MEL_SPACING};

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};
