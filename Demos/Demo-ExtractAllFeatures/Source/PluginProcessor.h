/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - Feature Extractor Plugin

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 10th September 2020

  ==============================================================================
*/


#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"

#define USE_AUBIO_ONSET //If this commented, the bark onset detector is used, otherwise the aubio onset module is used
// #define MEASURE_COMPUTATION_LATENCY
//==============================================================================
/**
*/
class DemoProcessor : public AudioProcessor,
                      public HighResolutionTimer,
#ifdef USE_AUBIO_ONSET
                      public tid::aubio::Onset<float>::Listener
#else
                      public tid::Bark<float>::Listener
#endif
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

    tid::Log logger{"knnDemo-"};

    const unsigned int WINDOW_SIZE = 1024;
    const float BARK_SPACING = 0.5;
    const float BARK_BOUNDARY = 8.5;
    const float MEL_SPACING = 100;
    //=================== Onset module initialization ==========================

   #ifdef USE_AUBIO_ONSET
    /**    Set initial parameters      **/
    const unsigned int HOP = 128;
    const float ONSET_THRESHOLD = 0.0f;
    const float ONSET_MINIOI = 0.2f;  //200 ms debounce
    const float SILENCE_THRESHOLD = -48.0f;
    const tid::aubio::OnsetMethod ONSET_METHOD = tid::aubio::OnsetMethod::defaultMethod;

    tid::aubio::Onset<float> aubioOnset{WINDOW_SIZE,HOP,ONSET_THRESHOLD,ONSET_MINIOI,SILENCE_THRESHOLD,ONSET_METHOD};

    void onsetDetected (tid::aubio::Onset<float> *);
   #else
    /**    Set initial parameters      **/
    const unsigned int HOP = 128;

    /**    Initialize the onset detector      **/
    tid::Bark<float> bark{WINDOW_SIZE, HOP, BARK_SPACING};
    void onsetDetected (tid::Bark<float>* bark);
   #endif
    void onsetDetectedRoutine();

    /**    Initialize the modules      **/

    tid::AttackTime<float> attackTime;
    tid::BarkSpecBrightness<float> barkSpecBrightness{WINDOW_SIZE, BARK_SPACING, BARK_BOUNDARY};
    tid::BarkSpec<float> barkSpec{WINDOW_SIZE, BARK_SPACING};
    tid::Bfcc<float> bfcc{WINDOW_SIZE, BARK_SPACING};
    tid::Cepstrum<float> cepstrum{this->WINDOW_SIZE};
    tid::Mfcc<float> mfcc{this->WINDOW_SIZE, this->MEL_SPACING};
    tid::PeakSample<float> peakSample{WINDOW_SIZE};
    tid::ZeroCrossing<float> zeroCrossing{WINDOW_SIZE};

    const unsigned int VECTOR_SIZE = 658;

    std::vector<float> featureVector;
    void computeFeatureVector();

    tid::KNNclassifier knn;

    /**    This atomic is used to update the onset LED in the GUI **/
    std::atomic<bool> onsetMonitorState{false};

    std::atomic<unsigned int> matchAtomic{0};
    std::atomic<float> distAtomic{-1.0f};

    std::atomic<bool> onsetWasDetected{false};

    enum class CState {
        idle,
        train,
        classify
    };

    CState classifierState = CState::idle;

    void hiResTimerCallback();

   #ifdef MEASURE_COMPUTATION_LATENCY
    double latencyTime = 0;
   #endif

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};
