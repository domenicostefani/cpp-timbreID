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

    this->jLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(logPath, LOG_FILENAME, LOG_EXTENSION, "guitarTimbreClassifier VST - log"));
    this->jLogger->logMessage("Starting log at: " + logPath);

    /** SET SOME DEFAULT PARAMETERS OF THE BARK MODULE **/
    bark.setDebounce(30);
    bark.setMask(4, 0.75);
    bark.setFilterRange(0, 49);
    bark.setThresh(30, 200);

    /** ADD THE LISTENER **/
    bark.addListener(this);

    /** SET UP CLASSIFIER **/
    this->jLogger->logMessage("Model path set to '" + TFLITE_MODEL_PATH + "'");
    this->timbreClassifier = createClassifier(TFLITE_MODEL_PATH);
    this->jLogger->logMessage("Classifier object istantiated");
}

DemoProcessor::~DemoProcessor(){
    deleteClassifier(this->timbreClassifier);
    this->jLogger->logMessage("Classifier object deleted");
}

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    /** PREPARE THE BARK MODULEs **/
    this->jLogger->logMessage("prepareToPlay() was called");
    bark.prepare(sampleRate, (uint32)samplesPerBlock);
    bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
    sinewt.prepareToPlay(sampleRate);
}

void DemoProcessor::releaseResources()
{
    /** RESET THE MODULEs **/
    this->jLogger->logMessage("releaseResources() was called");
    bark.reset();
    bfcc.reset();
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    /** STORE THE bfcc BUFFER FOR ANALYSIS **/
    bfcc.store(buffer,(short int)MONO_CHANNEL);

   #ifdef DEBUG_WHICH_CHANNEL
    if(logCounter > (48000/64)){
        logCounter = 0;
        this->jLogger->logMessage("process 0:" + std::to_string(*buffer.getReadPointer(0,0)) + " 1:" + std::to_string(*buffer.getReadPointer(1,0)));
    }
    logCounter++;
   #endif

    /** STORE THE bark BUFFER **/
    try
    {
        /** STORE THE BUFFER FOR COMPUTATION **/
        bark.store(buffer,(short int)MONO_CHANNEL);
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

    bool VERBOSE = true;
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
    auto start = std::chrono::high_resolution_clock::now();
    std::pair<int,float> result = classify(timbreClassifier,featureVector);
    auto stop = std::chrono::high_resolution_clock::now();

    this->jLogger->logMessage("Predicted class " + std::to_string(result.first) + " with confidence: " + std::to_string(result.second));
    this->jLogger->logMessage("It took " + std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count()) + "us");
    this->jLogger->logMessage("(or " + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count()) + "ms)");
    /*---------------------*/

    int prediction = std::get<0>(result);
    float confidence = std::get<1>(result);

    if (VERBOSERES)
    {
        this->jLogger->logMessage("Match: " + std::to_string(prediction));
        this->jLogger->logMessage("Confidence: " + std::to_string(confidence));
    }

    if(confidence > 0.90)
        sinewt.playNote((prediction+1) * 440);
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
