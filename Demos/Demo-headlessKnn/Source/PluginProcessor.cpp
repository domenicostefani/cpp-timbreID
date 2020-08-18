/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - bark Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <iostream>

// #define USE_TEXT_DATA
#define DATA_PATH "./data/traindata.timid"
#define LOG_PATH "/tmp"

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
    /** Starting log **/
    juce::String jpath(LOG_FOLDER_PATH);
    jpath = juce::File::addTrailingSeparator(jpath);
    std::string logPath = jpath.toStdString();

    this->jLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(logPath, LOG_FILENAME, LOG_EXTENSION, "headlessKnn VST - log"));
    this->jLogger->logMessage("Starting log at: " + logPath);

    /** SET SOME DEFAULT PARAMETERS OF THE BARK MODULE **/
    bark.setDebounce(30);
    bark.setMask(4, 0.75);
    bark.setFilterRange(0, 49);
    bark.setThresh(30, 200);

    /** ADD THE LISTENER **/
    bark.addListener(this);

    /** NOW FOR THE HARDCODED FUNCTIONALITIES **/
    bool readDataResult;
   #ifdef USE_TEXT_DATA
    readDataResult = this->knn.readTextData(DATA_PATH);
   #else
    readDataResult = this->knn.readData(DATA_PATH);
   #endif

    if(readDataResult)
    {
        std::vector<float> clusterout = this->knn.autoCluster(NUM_CLUSTERS);
        if( !clusterout.empty() )
        {
            this->classifierState = CState::classify;
        }
    }
}

DemoProcessor::~DemoProcessor(){    /*DESTRUCTOR*/        }

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    /** PREPARE THE BARK MODULEs **/
    bark.prepare(sampleRate, (uint32)samplesPerBlock);
    bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
    sinewt.prepareToPlay(sampleRate);
}

void DemoProcessor::releaseResources()
{
    /** RESET THE MODULEs **/
    bark.reset();
    bfcc.reset();
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    /** STORE THE bfcc BUFFER FOR ANALYSIS **/
    bfcc.store(buffer,(short int)0);

    /** STORE THE bark BUFFER **/
    try
    {
        /** STORE THE BUFFER FOR COMPUTATION and retrieving growthdata**/
        tid::Bark<float>::GrowthData growthData = bark.store(buffer,(short int)0);

        /** Retrieve the total growth and the growth data list **/
        std::vector<float>* growth = growthData.getGrowth();
        float* totalGrowth = growthData.getTotalGrowth();

        /** Check whether the data is valid: depending on analysis and
            whether spew mode is ON, the growthData will be provided
            at different calls
        **/
        if(growth != nullptr && totalGrowth != nullptr)
        {
            /** This is not a good example of how to log the list
                The standard stream output shouldn't be used from
                the audio thread.
            **/
#ifdef PRINT_GROWTH
            this->jLogger->logMessage(std::to_string(*totalGrowth) + " : ");

            std::string growth_s = "";
            for(int i=0; i<growth->size(); ++i)
            {
                growth_s += std::to_string(growth->at(i));
                if(i != growth->size()-1)
                    growth_s += ", ";
            }

            this->jLogger->logMessage(growth_s + "\n");
#endif // PRINT_GROWTH
        }

    }
    catch(std::exception& e)
    {
        this->jLogger->logMessage("An exception has occurred while buffering:");
        this->jLogger->logMessage(e.what());
    }
    catch(...)
    {
        this->jLogger->logMessage("An UNKNOWN exception has occurred while buffering");
    }

    sinewt.processBlock(buffer);
}

/**
 * Onset Detected Callback
 * Remember that this is called from the audio thread so it must not update the GUI directly
**/
void DemoProcessor::onsetDetected (tid::Bark<float> * bark)
{
    if(bark == &this->bark)
        this->onsetMonitorState.exchange(true);

    bool VERBOSE = false;
    bool VERBOSERES = true;

    if (VERBOSE) this->jLogger->logMessage("Onset Detected");

    std::vector<float> featureVector = bfcc.compute();

    if (VERBOSE) this->jLogger->logMessage("Bfcc computed");

    featureVector.resize(this->featuresUsed);

    if (VERBOSE) this->jLogger->logMessage("Feature vector size cut down to " + std::to_string(this->featuresUsed));

    if (VERBOSE)
    {
        std::string toprint = "";
        for(float val : featureVector)
            toprint += (std::to_string(val) + " ");
        this->jLogger->logMessage(toprint);
    }

    switch (classifierState)
    {

        case CState::idle:
            {
                if (VERBOSERES) this->jLogger->logMessage("detected onset but classifier is idling");
            }
            break;
        case CState::train:
            {
                unsigned int idAssigned = knn.trainModel(featureVector);
                matchAtomic.exchange(idAssigned);
                if (VERBOSERES)
                {
                    this->jLogger->logMessage("Trained model");
                }
            }
            break;
        case CState::classify:
            {
                auto res = knn.classifySample(featureVector);
                unsigned int prediction = std::get<0>(res[0]);
                float confidence = std::get<1>(res[0]);
                float distance = std::get<2>(res[0]);
                if (VERBOSERES)
                {
                    this->jLogger->logMessage("Match: " + std::to_string(prediction));
                    this->jLogger->logMessage("Confidence: " + std::to_string(confidence));
                    this->jLogger->logMessage("Distance: " + std::to_string(distance));
                }
                matchAtomic.exchange(prediction);
                distAtomic.exchange(distance);

                sinewt.playNote((prediction+1) * 440);
            }
            break;
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
