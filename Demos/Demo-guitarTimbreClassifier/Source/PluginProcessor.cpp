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


#define MONO_CHANNEL 0
#define POST_ONSET_DELAY_MS 7.33
#define DO_DELAY_ONSET // If commented there is no delay between onset detection and feature extraction (bad)

//==============================================================================
DemoProcessor::DemoProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::mono(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::mono(), true)
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

    featureVector.resize(VECTOR_SIZE);

    /** SET UP CLASSIFIER **/
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];
    snprintf(message,sizeof(message),"Model path set to '%s'",TFLITE_MODEL_PATH.c_str());
    rtlogger.logInfo(message);
    this->timbreClassifier = createClassifier(TFLITE_MODEL_PATH);
    rtlogger.logInfo("Classifier object istantiated");
    // Start the polling routine that writes to file all the log entries
    pollingTimer.startLogRoutine(100); // Call every 0.1 seconds
}

DemoProcessor::~DemoProcessor(){
    deleteClassifier(this->timbreClassifier);
    rtlogger.logInfo("Classifier object deleted");
    logPollingRoutine(); // Log last messages before closing
}

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    rtlogger.logInfo("+--Prepare to play called");
    rtlogger.logInfo("+  Setting up onset detector");

   #ifdef USE_AUBIO_ONSET
    /** PREPARE THE BARK MODULEs **/
    aubioOnset.prepare(sampleRate,samplesPerBlock); // Important step
   #else
    /** PREPARE THE BARK MODULEs **/
    bark.prepare(sampleRate, (uint32)samplesPerBlock);
   #endif

   #ifdef DO_DELAY_ONSET
    postOnsetTimer.prepare(sampleRate,samplesPerBlock);
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

    rtlogger.logInfo("+--Prepare to play completed");
}

void DemoProcessor::releaseResources()
{
    rtlogger.logInfo("+--ReleaseResources called");

    rtlogger.logInfo("+  Releasing Onset detector resources");
    /** RESET THE MODULEs **/
   #ifdef USE_AUBIO_ONSET
    aubioOnset.reset(); // Important step
   #else
    bark.reset();
   #endif
    rtlogger.logInfo("+  Releasing extractor resources");

    // attackTime.reset();
    // barkSpecBrightness.reset();
    // barkSpec.reset();
    bfcc.reset();
    cepstrum.reset();
    // mfcc.reset();
    // peakSample.reset();
    // zeroCrossing.reset();

    rtlogger.logInfo("+--ReleaseResources completed");
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH]; // logger message

   #ifdef DEBUG_WHICH_CHANNEL
    if(logCounter > (48000/64)){
        logCounter = 0;
        for(int c = 0; c < buffer.getNumChannels(); ++c)
        {
            snprintf(message,sizeof(message),"DEBUG | Channel %d, first sample: %f ",c,*buffer.getReadPointer(c,0));
            rtlogger.logInfo(message);
        }
    }
    logCounter++;
   #endif

   #ifdef MEASURE_COMPUTATION_LATENCY
    int64 timeAtBlockStart = 0;
    if (++logProcessblock_counter >= logProcessblock_period)
    {
        logProcessblock_fixedCounter += logProcessblock_counter; // Add the number of processed blocks to the total
        logProcessblock_counter = 0;
        timeAtBlockStart = Time::Time::getHighResolutionTicks();
    }

    int64 timeAtClassificationStart = 0;
   #endif

    if(postOnsetTimer.isExpired())
    {
       #ifdef MEASURE_COMPUTATION_LATENCY
        timeAtClassificationStart = Time::Time::getHighResolutionTicks();
       #endif
        onsetDetectedRoutine();
    }

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

   #ifdef MEASURE_COMPUTATION_LATENCY
    if(timeAtClassificationStart)   // If this time a classification is happening, We are going to log it
    {
        snprintf(message,sizeof(message),"Classification round ");
        rtlogger.logInfo(message,timeAtClassificationStart,Time::Time::getHighResolutionTicks());
        timeAtClassificationStart = 0;
    }

    if (timeAtBlockStart)   // If timeAtBlockStart was initialized this round, we are computing and logging processBlock Duration
    {
        snprintf(message,sizeof(message),"block nr %d processed",logProcessblock_fixedCounter);
        rtlogger.logInfo(message,timeAtBlockStart,Time::Time::getHighResolutionTicks());
        timeAtBlockStart = 0;
    }
   #endif

   #ifdef DO_DELAY_ONSET
    postOnsetTimer.updateTimer(); // Update the block counter of the postOnsetTimer
   #endif
}

#ifdef USE_AUBIO_ONSET
void DemoProcessor::onsetDetected (tid::aubio::Onset<float> *aubioOnset){
    if(aubioOnset == &this->aubioOnset)
    {
#else
void DemoProcessor::onsetDetected (tid::Bark<float> * bark)
{
    if(bark == &this->bark)
    {
#endif
       #ifdef MEASURE_COMPUTATION_LATENCY
        latencyTime = juce::Time::getMillisecondCounterHiRes();
       #endif

       #ifdef DO_DELAY_ONSET
        rtlogger.logValue("Start waiting for ",(float)POST_ONSET_DELAY_MS,"ms"); // TODO: remove
        if(postOnsetTimer.isIdle())
            postOnsetTimer.start(POST_ONSET_DELAY_MS);
       #else
        onsetDetectedRoutine();
       #endif
    }
}

/**
 * Onset Detected Callback
 * Remember that this is called from the audio thread so it must not update the GUI directly
**/
void DemoProcessor::onsetDetectedRoutine ()
{
    rtlogger.logInfo("Onset detected");
   #ifdef DO_DELAY_ONSET
    rtlogger.logInfo("delay applied");
   #endif

    bool VERBOSE = false;
    bool VERBOSERES = true;

   #ifdef MEASURE_COMPUTATION_LATENCY
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];
    snprintf(message,sizeof(message),"Computation started %f ms after onset detection",(juce::Time::getMillisecondCounterHiRes() - latencyTime));
    rtlogger.logInfo(message);
   #endif

    this->computeFeatureVector();

   #ifdef MEASURE_COMPUTATION_LATENCY
    snprintf(message,sizeof(message),"Computation stopped %f ms after onset detection",(juce::Time::getMillisecondCounterHiRes() - latencyTime));
    rtlogger.logInfo(message);
   #endif


    std::string predictionInfo = ""; //TODO check if this is bad
    predictionInfo.reserve(10000);   //TODO at least this should be good
    auto start = std::chrono::high_resolution_clock::now();
    std::pair<int,float> result = classify(timbreClassifier,featureVector,predictionInfo);
    auto stop = std::chrono::high_resolution_clock::now();

    // rtlogger.logInfo("----\n" + predictionInfo + "\n----"); //TODO fix
    snprintf(message,sizeof(message),"Predicted class %d with confidence %f",result.first,result.second);
    rtlogger.logInfo(message);
    snprintf(message,sizeof(message),"It took %d us\nor %d ms", \
        std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count(),\
        std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());
    rtlogger.logInfo(message);
    /*---------------------*/

   #ifdef MEASURE_COMPUTATION_LATENCY
    snprintf(message,sizeof(message),"Prediction stopped %f ms after onset detection",(juce::Time::getMillisecondCounterHiRes() - latencyTime));
    rtlogger.logInfo(message);
   #endif

    int prediction = std::get<0>(result);
    float confidence = std::get<1>(result);

    if (VERBOSERES)
    {
        snprintf(message,sizeof(message),"Match: %d\nConfidence: %f", prediction,confidence);
        rtlogger.logInfo(message);
    }

    // if(confidence > 0.90 && prediction != 4)
    if(prediction != 4)
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
