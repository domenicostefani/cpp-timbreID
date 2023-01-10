/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - Headless Classifier

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"
#include "wavetableSine.h"

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

    //===================== module initialization =============================

    /**    Set initial parameters      **/
    const unsigned int WINDOW_SIZE = 1024; //previous 2048
    const unsigned int HOP = 128f;
    const float BARK_SPACING = 0.5f;

    /**    Initialize the modules      **/
    tid::Bark<float> bark{WINDOW_SIZE, HOP, BARK_SPACING};
    tid::Bfcc<float> bfcc{WINDOW_SIZE, BARK_SPACING};
    tid::KNNclassifier knn{};

    /**    This atomic is used to update the onset LED in the GUI **/
    std::atomic<bool> onsetMonitorState{false};

    std::atomic<unsigned int> matchAtomic{0};
    std::atomic<float> distAtomic{-1.0f};

    /**    Only the first n features of bfcc are used  **/
    int featuresUsed = 25;

    enum class CState {
        idle,
        train,
        classify
    };

    CState classifierState = CState::idle;

    const int NUM_CLUSTERS = 4;
    const int KVALUE = 3;

    /** LOG */
    std::unique_ptr<FileLogger> jLogger;
    const std::string LOG_FOLDER_PATH = "/tmp/";
    const std::string LOG_FILENAME = "headlessKnn";
    const std::string LOG_EXTENSION = ".log";

    uint32 logCounter = 0;


private:
    /* Output sound */
    WavetableSine sinewt;

    void onsetDetected (tid::Bark<float>* bark);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};
