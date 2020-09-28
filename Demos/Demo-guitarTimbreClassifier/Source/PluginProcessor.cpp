/*
  ==============================================================================
     Plugin Processor

     DEMO PROJECT - TimbreID - Guitar Timbre Classifier

     Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
     Date: 24th August 2020
  ==============================================================================
*/
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "liteclassifier.h"

#include <vector>
#include <chrono>  //TODO: remove


#define MONO_CHANNEL 1
// #define DEBUG_WHICH_CHANNEL
#define MAXIMUM_LATENCY_MSEC 10
#define DO_DELAY_ONSET // If commented there is no delay between onset detection and feature extraction (bad)

//==============================================================================
DemoProcessor::DemoProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    logger.logInfo("Initializing Onset detector");

   #ifdef USE_AUBIO_ONSET
    /** ADD THE LISTENER **/
    aubioOnset.addListener(this);
   #else
    /** SET SOME DEFAULT PARAMETERS OF THE BARK MODULE **/
    bark.setDebounce(200);
    bark.setMask(4, 0.75);
    bark.setFilterRange(0, 49);
    bark.setThresh(-1, 30);

    /** ADD THE LISTENER **/
    bark.addListener(this);
   #endif

    featureVector.resize(VECTOR_SIZE);

    /** SET UP CLASSIFIER **/
    logger.logInfo("Model path set to '" + TFLITE_MODEL_PATH + "'");
    this->timbreClassifier = createClassifier(TFLITE_MODEL_PATH);
    logger.logInfo("Classifier object istantiated");
}

DemoProcessor::~DemoProcessor(){
    deleteClassifier(this->timbreClassifier);
    logger.logInfo("Classifier object deleted");
}

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    logger.logInfo("+--Prepare to play called");
    logger.logInfo("+  Setting up onset detector");

   #ifdef USE_AUBIO_ONSET
    /** PREPARE THE BARK MODULEs **/
    aubioOnset.prepare(sampleRate,samplesPerBlock); // Important step
   #else
    /** PREPARE THE BARK MODULEs **/
    bark.prepare(sampleRate, (uint32)samplesPerBlock);
   #endif

    // attackTime.prepare(sampleRate, (uint32)samplesPerBlock);
    // barkSpecBrightness.prepare(sampleRate, (uint32)samplesPerBlock);
    // barkSpec.prepare(sampleRate, (uint32)samplesPerBlock);
    bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
    cepstrum.prepare(sampleRate, (uint32)samplesPerBlock);
    // mfcc.prepare(sampleRate, (uint32)samplesPerBlock);
    // peakSample.prepare(sampleRate, (uint32)samplesPerBlock);
    // zeroCrossing.prepare(sampleRate, (uint32)samplesPerBlock);

    sinewt.prepareToPlay(sampleRate);

    logger.logInfo("+--Prepare to play completed");
}

void DemoProcessor::releaseResources()
{
    logger.logInfo("+--ReleaseResources called");

    logger.logInfo("+  Releasing Onset detector resources");
    /** RESET THE MODULEs **/
   #ifdef USE_AUBIO_ONSET
    aubioOnset.reset(); // Important step
   #else
    bark.reset();
   #endif
    logger.logInfo("+  Releasing extractor resources");

    // attackTime.reset();
    // barkSpecBrightness.reset();
    // barkSpec.reset();
    bfcc.reset();
    cepstrum.reset();
    // mfcc.reset();
    // peakSample.reset();
    // zeroCrossing.reset();

    logger.logInfo("+--ReleaseResources completed");
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

   #ifdef DEBUG_WHICH_CHANNEL
    if(logCounter > (48000/64)){
        logCounter = 0;
        logger.logInfo("process 0:" + std::to_string(*buffer.getReadPointer(0,0)) + " 1:" + std::to_string(*buffer.getReadPointer(1,0)));
    }
    logCounter++;
   #endif

    if(this->onsetWasDetected.exchange(false))
       onsetDetectedRoutine();

    /** STORE THE BUFFER FOR ANALYSIS **/
    // attackTime.store(buffer,(short int)MONO_CHANNEL);
    // barkSpecBrightness.store(buffer,(short int)MONO_CHANNEL);
    // barkSpec.store(buffer,(short int)MONO_CHANNEL);
    bfcc.store(buffer,(short int)MONO_CHANNEL);
    cepstrum.store(buffer,(short int)MONO_CHANNEL);
    // mfcc.store(buffer,(short int)MONO_CHANNEL);
    // peakSample.store(buffer,(short int)MONO_CHANNEL);
    // zeroCrossing.store(buffer,(short int)MONO_CHANNEL);

    /** STORE THE ONSET BUFFER **/
    try
    {
       #ifdef USE_AUBIO_ONSET
        aubioOnset.store(buffer,MONO_CHANNEL);
       #else
        /** STORE THE BUFFER FOR COMPUTATION **/
        bark.store(buffer,(short int)MONO_CHANNEL);
       #endif
    }
    catch(std::exception& e)
    {
        std::cout << "An exception has occurred while buffering:" << std::endl;
        std::cout << e.what() << '\n';
    }
    catch(...)
    {
        std::cout << "An UNKNOWN exception has occurred while buffering" << std::endl;
    }
    // buffer.clear();
    sinewt.processBlock(buffer);
}

#ifdef USE_AUBIO_ONSET
void DemoProcessor::onsetDetected (tid::aubio::Onset<float> *aubioOnset){
    if(aubioOnset == &this->aubioOnset)
    {
       #ifdef MEASURE_COMPUTATION_LATENCY
        latencyTime = juce::Time::getMillisecondCounterHiRes();
       #endif

       #ifdef DO_DELAY_ONSET
        /* We assume that there could be a delay of one entire Hop window */
        const unsigned int SAMPLES_FROM_PEAK = this->aubioOnset.getHopSize();
        float delayMsec = (bfcc.getWindowSize()-SAMPLES_FROM_PEAK) / this->getSampleRate() * 1000;
        /* Check that the timer does not interfere with successive onsets */
        float minIoiMsec = this->aubioOnset.getOnsetMinInterOnsetInterval()*1000.0;
        /* Check that complessive delay does not go over MAXIMUM_LATENCY_MSEC milliseconds */
        float ms_remaining_to_maxlatency = MAXIMUM_LATENCY_MSEC - (SAMPLES_FROM_PEAK / this->getSampleRate() * 1000);
        ms_remaining_to_maxlatency = ms_remaining_to_maxlatency < 0 ? 0 : ms_remaining_to_maxlatency;

        float realtimeThresh = ms_remaining_to_maxlatency < minIoiMsec ? ms_remaining_to_maxlatency : minIoiMsec;

       #ifdef MEASURE_COMPUTATION_LATENCY
        logger.logInfo("Set timer for " + std::to_string(delayMsec < realtimeThresh ? delayMsec : realtimeThresh) + "ms");
       #endif
        HighResolutionTimer::startTimer(delayMsec < realtimeThresh ? delayMsec : realtimeThresh);
       #else
        onsetDetectedRoutine();
       #endif
    }
}
#else
void DemoProcessor::onsetDetected (tid::Bark<float> * bark)
{
    if(bark == &this->bark)
    {
       #ifdef DO_DELAY_ONSET
        //This is an assumption on the magnitude of the samplesFromPeak
        unsigned int samplesFromPeak = this->bark.getHop() * 2; //TODO: fix as soon as the bark mods are confirmed

        float delayMsec = (bfcc.getWindowSize()-samplesFromPeak) / this->getSampleRate() * 1000;
        float realtimeThresh = MAXIMUM_LATENCY_MSEC - (samplesFromPeak/this->getSampleRate() * 1000);
        HighResolutionTimer::startTimer(delayMsec < realtimeThresh ? delayMsec : realtimeThresh);
       #else
        onsetDetectedRoutine();
       #endif
    }
}
#endif

void DemoProcessor::hiResTimerCallback(){
    HighResolutionTimer::stopTimer();
    this->onsetWasDetected.exchange(true);
   #ifdef MEASURE_COMPUTATION_LATENCY
    logger.logInfo("Timer stopped after " + std::to_string(juce::Time::getMillisecondCounterHiRes() - latencyTime));
   #endif
}


/**
 * Onset Detected Callback
 * Remember that this is called from the audio thread so it must not update the GUI directly
**/
void DemoProcessor::onsetDetectedRoutine ()
{
    logger.logInfo("Onset detected");
   #ifdef DO_DELAY_ONSET
    logger.logInfo("delay applied");
   #endif

    this->onsetMonitorState.exchange(true);

    bool VERBOSE = false;
    bool VERBOSERES = true;

   #ifdef MEASURE_COMPUTATION_LATENCY
    logger.logInfo("Computation started after " + std::to_string(juce::Time::getMillisecondCounterHiRes() - latencyTime));
   #endif

    this->computeFeatureVector();

   #ifdef MEASURE_COMPUTATION_LATENCY
    logger.logInfo("Computation stopped after " + std::to_string(juce::Time::getMillisecondCounterHiRes() - latencyTime));
   #endif

    if (VERBOSE)
    {
        std::string toprint = "";
        for(float val : featureVector)
            toprint += (std::to_string(val) + " ");
        logger.logInfo(toprint);
    }
    std::string predictionInfo = "";

    auto start = std::chrono::high_resolution_clock::now();
    std::pair<int,float> result = classify(timbreClassifier,featureVector,predictionInfo);
    auto stop = std::chrono::high_resolution_clock::now();

    logger.logInfo("----\n" + predictionInfo + "\n----");
    logger.logInfo("Predicted class " + std::to_string(result.first) + " with confidence: " + std::to_string(result.second));
    logger.logInfo("It took " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()) + "us");
    logger.logInfo("(or " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) + "ms)");
    /*---------------------*/

    int prediction = std::get<0>(result);
    float confidence = std::get<1>(result);

    if (VERBOSERES)
    {
        logger.logInfo("Match: " + std::to_string(prediction));
        logger.logInfo("Confidence: " + std::to_string(confidence));
    }

    if(confidence > 0.90 && prediction != 4)
        sinewt.playNote((prediction+1) * 440);
}

void DemoProcessor::computeFeatureVector()
{
    /* Bfcc */
    std::vector<float> bfccRes = this->bfcc.compute();
    const int BFCC_RES_SIZE = 50;
    jassert(bfccRes.size() == BFCC_RES_SIZE);
    for(int i=0; i<BFCC_RES_SIZE; ++i)
    {
        this->featureVector[i] = bfccRes[i];
    }

    /* First 60 Cespstrum coefficients */
    std::vector<float> cepstrumRes = this->cepstrum.compute();
    const int CEPSTRUM_RES_SIZE = 513;
    jassert(cepstrumRes.size() == CEPSTRUM_RES_SIZE);
    for(int i=0; i<60; ++i)
    {
        this->featureVector[50 + i] = cepstrumRes[i];
    }
}

/*    JUCE stuff ahead, not relevant to the demo    */












//==============================================================================
const String DemoProcessor::getName() const{return JucePlugin_Name;}
bool DemoProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DemoProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}
bool DemoProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}
double DemoProcessor::getTailLengthSeconds() const{return 0.0;}
int DemoProcessor::getNumPrograms(){return 1;}
int DemoProcessor::getCurrentProgram(){return 0;}
void DemoProcessor::setCurrentProgram (int index){}
const String DemoProcessor::getProgramName (int index){return {};}
void DemoProcessor::changeProgramName (int index, const String& newName){}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool DemoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


//==============================================================================
bool DemoProcessor::hasEditor() const {return true;}
AudioProcessorEditor* DemoProcessor::createEditor(){return new DemoEditor (*this);}
void DemoProcessor::getStateInformation (MemoryBlock& destData){}
void DemoProcessor::setStateInformation (const void* data, int sizeInBytes){}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemoProcessor();
}
