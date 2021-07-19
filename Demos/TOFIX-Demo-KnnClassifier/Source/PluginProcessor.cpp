/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - Training + classification with knn

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <iostream>

#define MONO_CHANNEL 0
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
    rtlogger.logInfo("Initializing Onset detector");

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
}

DemoProcessor::~DemoProcessor(){    /*DESTRUCTOR*/        }

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    rtlogger.logInfo("Prepare to play called");
    rtlogger.logInfo("Setting up onset detector");

   #ifdef USE_AUBIO_ONSET
    /** PREPARE THE BARK MODULEs **/
    aubioOnset.prepare(sampleRate,samplesPerBlock); // Important step
   #else
    /** PREPARE THE BARK MODULEs **/
    bark.prepare(sampleRate, (uint32)samplesPerBlock);
   #endif

    rtlogger.logInfo("Setting up BFCC extractor");
    bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
}

void DemoProcessor::releaseResources()
{
    rtlogger.logInfo("ReleaseResources called");


    rtlogger.logInfo("Releasing Onset detector resources");
    /** RESET THE MODULEs **/
   #ifdef USE_AUBIO_ONSET
    aubioOnset.reset(); // Important step
   #else
    bark.reset();
   #endif
    rtlogger.logInfo("Releasing BFCC extractor resources");
    bfcc.reset();
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    // ScopedNoDenormals noDenormals;

    // if(this->onsetWasDetected.exchange(false))
    //     onsetDetectedRoutine();

    /** STORE THE bfcc BUFFER FOR ANALYSIS **/
    bfcc.store(buffer,(short int)MONO_CHANNEL);

    // /** STORE THE ONSET BUFFER **/
    // try
    // {
    //    #ifdef USE_AUBIO_ONSET
          //  aubioOnset.store(buffer,MONO_CHANNEL);
    //    #else
    //     /** STORE THE BUFFER FOR COMPUTATION **/
    //     bark.store(buffer,(short int)MONO_CHANNEL);
    //    #endif
    // }
    // catch(std::exception& e)
    // {
    //     std::cout << "An exception has occurred while buffering:" << std::endl;
    //     std::cout << e.what() << '\n';
    // }
    // catch(...)
    // {
    //     std::cout << "An UNKNOWN exception has occurred while buffering" << std::endl;
    // }
    // //buffer.clear();
}

#ifdef USE_AUBIO_ONSET
void DemoProcessor::onsetDetected (tid::aubio::Onset<float> *aubioOnset){
    // if(aubioOnset == &this->aubioOnset)
    // {
    //    #ifdef DO_DELAY_ONSET
    //     /* We assume that there could be a delay of one entire Hop window */
    //     const unsigned int SAMPLES_FROM_PEAK = this->aubioOnset.getHopSize();
    //     float delayMsec = (bfcc.getWindowSize()-SAMPLES_FROM_PEAK) / this->getSampleRate() * 1000;
    //     /* Check that the timer does not interfere with successive onsets */
    //     float minIoiMsec = this->aubioOnset.getOnsetMinInterOnsetInterval()*1000.0;
    //     /* Check that complessive delay does not go over MAXIMUM_LATENCY_MSEC milliseconds */
    //     float ms_remaining_to_maxlatency = MAXIMUM_LATENCY_MSEC - (SAMPLES_FROM_PEAK / this->getSampleRate() * 1000);
    //     ms_remaining_to_maxlatency = ms_remaining_to_maxlatency < 0 ? 0 : ms_remaining_to_maxlatency;

    //     float realtimeThresh = ms_remaining_to_maxlatency < minIoiMsec ? ms_remaining_to_maxlatency : minIoiMsec;

    //     Timer::startTimer(delayMsec < realtimeThresh ? delayMsec : realtimeThresh);
    //    #else
    //     onsetDetectedRoutine();
    //    #endif
    // }
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
        Timer::startTimer(delayMsec < realtimeThresh ? delayMsec : realtimeThresh);
       #else
        onsetDetectedRoutine();
       #endif
    }
}
#endif

void DemoProcessor::timerCallback(){
//     Timer::stopTimer();
//     this->onsetWasDetected.exchange(true);
}

/**
 * Onset Detected Callback
 * Remember that this is called from the audio thread so it must not update the GUI directly
**/
// void DemoProcessor::onsetDetectedRoutine ()
// {
//     rtlogger.logInfo("Onset detected");
//    #ifdef DO_DELAY_ONSET
//     rtlogger.logInfo("delay applied");
//    #endif

//     this->onsetMonitorState.exchange(true);

//     bool VERBOSE = true;
//     bool VERBOSERES = true;

//     std::vector<float> featureVector = bfcc.compute();

//     if (VERBOSE) rtlogger.logInfo("Bfcc computed");

//     featureVector.resize(this->featuresUsed);

//     if (VERBOSE) rtlogger.logValue("Feature vector size cut down to ", this->featuresUsed);

//     if (VERBOSE)
//     {
//         std::string featVecStr = "feature vector: ";
//         for(float val : featureVector)
//             featVecStr += (std::to_string(val) + " ");
//         rtlogger.logInfo(featVecStr.c_str());
//     }

//     switch (classifierState)
//     {

//         case CState::idle:
//             if (VERBOSERES) rtlogger.logInfo("detected onset but classifier is idling");
//             break;
//         case CState::train:
//             {
//                 unsigned int idAssigned = knn.trainModel(featureVector);
//                 matchAtomic.exchange(idAssigned);
//                 if (VERBOSERES)
//                 {
//                     rtlogger.logInfo("Trained model");
//                 }
//             }
//             break;
//         case CState::classify:
//             {
//                 auto res = knn.classifySample(featureVector);
//                 unsigned int prediction = std::get<0>(res[0]);
//                 float confidence = std::get<1>(res[0]);
//                 float distance = std::get<2>(res[0]);
//                 if (VERBOSERES)
//                 {
//                     rtlogger.logValue("Match: ", prediction);
//                     rtlogger.logValue("Confidence: ", confidence);
//                     rtlogger.logValue("Distance: ", distance);
//                 }
//                 matchAtomic.exchange(prediction);
//                 distAtomic.exchange(distance);
//             }
//             break;
//     }
// }
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
