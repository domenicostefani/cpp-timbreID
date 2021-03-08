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
#include <chrono>

#define MONO_CHANNEL 0
#define POST_ONSET_DELAY_MS 7.33
#define DO_DELAY_ONSET // If not defined there is NO delay between onset detection and feature extraction

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
    /** ADD ONSET DETECTOR LISTENER **/
    aubioOnset.addListener(this);
   #else
    /** SET SOME DEFAULT PARAMETERS OF THE BARK MODULE **/
    bark.setDebounce(200);
    bark.setMask(4, 0.75);
    bark.setFilterRange(0, 49);
    bark.setThresh(-1, 30);

    /** ADD ONSET DETECTOR LISTENER **/
    bark.addListener(this);
   #endif

    /** ALLOCATE MEMORY FOR FEATURE VECTOR AND TMP VECTORS **/
    featureVector.resize(VECTOR_SIZE);
    bfccRes.resize(BFCC_RES_SIZE);
    cepstrumRes.resize(CEPSTRUM_RES_SIZE);

    /** SET UP CLASSIFIER **/
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];
    snprintf(message,sizeof(message),"Model path set to '%s'",TFLITE_MODEL_PATH.c_str());
    rtlogger.logInfo(message);
    classificationOutputVector.resize(N_OUTPUT_CLASSES,0.0f);
    this->timbreClassifier = createClassifier(TFLITE_MODEL_PATH);
    rtlogger.logInfo("Classifier object istantiated");

    /** START POLLING ROUTINE THAT WRITES TO FILE ALL THE LOG ENTRIES **/
    pollingTimer.startLogRoutine(100); // Call every 0.1 seconds
}

DemoProcessor::~DemoProcessor(){
    /** Free classifier memory **/
    deleteClassifier(this->timbreClassifier);
    rtlogger.logInfo("Classifier object deleted");

    /** Log last queued messages before closing **/
    logPollingRoutine();
}

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    rtlogger.logInfo("+--Prepare to play called");
    rtlogger.logInfo("+  Setting up onset detector");

    /** INIT ONSET DETECTOR MODULE**/
   #ifdef USE_AUBIO_ONSET
    aubioOnset.prepare(sampleRate,samplesPerBlock);
   #else
    bark.prepare(sampleRate, (uint32)samplesPerBlock);
   #endif

    /** INIT POST ONSET TIMER **/
   #ifdef DO_DELAY_ONSET
    postOnsetTimer.prepare(sampleRate,samplesPerBlock);
   #endif

    /*------------------------------------/
    | Prepare feature extractors          |
    /------------------------------------*/
    bfcc.prepare(sampleRate, (uint32)samplesPerBlock);      // Bark Frequency Cepstral Coefficients
    cepstrum.prepare(sampleRate, (uint32)samplesPerBlock);  // Cepstrum Coefficients

    //
    // Other Feature extractors available:
    //

    /*------------------------------------/
    | Attack time                         |
    /------------------------------------*/
    // attackTime.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Bark spectral brightness            |
    /------------------------------------*/
    // barkSpecBrightness.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Bark Spectrum                       |
    /------------------------------------*/
    // barkSpec.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    // mfcc.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    // peakSample.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    // zeroCrossing.prepare(sampleRate, (uint32)samplesPerBlock);

    sinewt.prepareToPlay(sampleRate);

    rtlogger.logInfo("+--Prepare to play completed");
}

void DemoProcessor::releaseResources()
{
    rtlogger.logInfo("+--ReleaseResources called");

    rtlogger.logInfo("+  Releasing Onset detector resources");

    /** FREE ONSET DETECTOR MEMORY **/
   #ifdef USE_AUBIO_ONSET
    aubioOnset.reset();
   #else
    bark.reset();
   #endif
    rtlogger.logInfo("+  Releasing extractor resources");

    /*------------------------------------/
    | Reset the feature extractors        |
    /------------------------------------*/
    bfcc.reset();
    cepstrum.reset();

    /*------------------------------------/
    | Other Feature extractors available  |
    /------------------------------------*/

    // attackTime.reset();
    // barkSpecBrightness.reset();
    // barkSpec.reset();
    // mfcc.reset();
    // peakSample.reset();
    // zeroCrossing.reset();

    rtlogger.logInfo("+--ReleaseResources completed");
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH]; // logger message

    /** LOG THE DIFFERENT TIMESTAMPS FOR LATENCY COMPUTATION **/
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

    /** EXTRACT FEATURES AND CLASSIFY IF POST ONSET TIMER IS EXPIRED **/
    if(postOnsetTimer.isExpired())
    {
       #ifdef MEASURE_COMPUTATION_LATENCY
        timeAtClassificationStart = Time::Time::getHighResolutionTicks();
       #endif
        onsetDetectedRoutine();
    }

    /** STORE THE BUFFER FOR FEATURE EXTRACTION **/
    bfcc.store(buffer,(short int)MONO_CHANNEL);
    cepstrum.store(buffer,(short int)MONO_CHANNEL);
    // attackTime.store(buffer,(short int)MONO_CHANNEL);           // < /*-----------------------------------/
    // barkSpecBrightness.store(buffer,(short int)MONO_CHANNEL);   // < |                                    |
    // barkSpec.store(buffer,(short int)MONO_CHANNEL);             // < | Other Feature extractors available |
    // mfcc.store(buffer,(short int)MONO_CHANNEL);                 // < |                                    |
    // peakSample.store(buffer,(short int)MONO_CHANNEL);           // < |                                    |
    // zeroCrossing.store(buffer,(short int)MONO_CHANNEL);         // < /-----------------------------------*/

    /** STORE THE ONSET DETECTOR BUFFER **/
    try
    {
       #ifdef USE_AUBIO_ONSET
        aubioOnset.store(buffer,MONO_CHANNEL);
       #else
        /** STORE THE BUFFER FOR COMPUTATION **/
        bark.store(buffer,(short int)MONO_CHANNEL);
       #endif
    } catch(std::exception& e) {
        rtlogger.logInfo("An exception has occurred while buffering: ",e.what());
    } catch(...) {
        rtlogger.logInfo("An unknwn exception has occurred while buffering: ");
    }
    /** CLEAR THE BUFFER (OPTIONAL) **/
    // buffer.clear(); // Uncomment to let input pass through

    /** ADD OUTPUT SOUND (TRIGGERED BY TIMBRE CLASSIFICATION) **/
    sinewt.processBlock(buffer);

    /** UPDATE POST ONSET COUNTER/TIMER **/
   #ifdef DO_DELAY_ONSET
    postOnsetTimer.updateTimer(); // Update the block counter of the postOnsetTimer
   #endif

    /** lOG LAST TIMESTAMPS FOR LATENCY COMPUTATION **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    if(timeAtClassificationStart)   // If this time a classification is happening, We are going to log it
    {
        rtlogger.logInfo("Classification round",timeAtClassificationStart,Time::Time::getHighResolutionTicks());
        timeAtClassificationStart = 0;
    }

    if (timeAtBlockStart)   // If timeAtBlockStart was initialized this round, we are computing and logging processBlock Duration
    {
        snprintf(message,sizeof(message),"block nr %d processed",logProcessblock_fixedCounter);
        rtlogger.logInfo(message,timeAtBlockStart,Time::Time::getHighResolutionTicks());
        timeAtBlockStart = 0;
    }
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
        rtlogger.logValue("Start waiting for ",(float)POST_ONSET_DELAY_MS,"ms");
        if(postOnsetTimer.isIdle())
            postOnsetTimer.start(POST_ONSET_DELAY_MS);
       #else
        onsetDetectedRoutine();
       #endif
    }
}

/**
 * Onset Detected Callback
 *
 * TODO: Measure accurately the time required by this function along
 *       with the buffering operations in the processBlock routine
 *       and consider performing this on a separate RT thread
 *       (At least classification if feature ext is not possible)
 *       ((Check TWINE library for Elk))
**/
void DemoProcessor::onsetDetectedRoutine ()
{
    bool VERBOSERES = true; // Log info about classification result (Suggested: true)
    bool VERBOSE = false;   // Log general verbose info (Suggested: false)

    rtlogger.logInfo("Onset detected");
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];

    /** LOG BEGINNING OF FEATURE EXTRACTION **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    rtlogger.logValue("Feature extraction started",(juce::Time::getMillisecondCounterHiRes() - latencyTime),"ms after onset detection");
   #endif

    /*--------------------/
    | 1. EXTRACT FEATURES |
    /--------------------*/
    this->computeFeatureVector();

    /** LOG ENDING OF FEATURE EXTRACTION **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    rtlogger.logValue("Feature extraction stopped",(juce::Time::getMillisecondCounterHiRes() - latencyTime),"ms after onset detection");
   #endif


    /*-------------------/
    | 2. CLASSIFY TIMBRE |
    /-------------------*/
    /** START ADDITIONAL "CHRONOMETER" JUST FOR CLASSIFICATION **/
    auto start = std::chrono::high_resolution_clock::now();
    /** INVOKE CLASSIFIER**/
    std::pair<int,float> result = classify(timbreClassifier,featureVector,classificationOutputVector);
    /** STOP CLASSIFICATION "CHRONOMETER" **/
    auto stop = std::chrono::high_resolution_clock::now();

    /** LOG ADDITIONAL TIMESTAMP FOR LATENCY COMPUTATION
        TODO: Improve logg clarity, precision and remove duplicates (like chrono library) **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    snprintf(message,sizeof(message),"Prediction stopped %f ms after onset detection",(juce::Time::getMillisecondCounterHiRes() - latencyTime));
    rtlogger.logInfo(message);
   #endif

    int prediction = result.first;
    int confidence = result.second;

    /** LOG CLASSIFICATION RESULTS AND TIME **/
    if (VERBOSERES)
    {
        snprintf(message,sizeof(message),"Result: Predicted class %d with confidence %f",prediction,confidence);
        rtlogger.logInfo(message);
        snprintf(message,sizeof(message),"Classification took %d us or %d ms", \
            std::chrono::duration_cast<std::chrono::microseconds>(stop - start).count(),\
            std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count());
        rtlogger.logInfo(message);
    }

    /** LOG ADDITIONAL INFO (CONFIDENCE VALUES FOR ALL CLASSES) **/
    if (VERBOSE)
    {
        for (int j = 0; j < N_OUTPUT_CLASSES; ++j)
        {
            snprintf(message,sizeof(message),"Class %d confidence %f",j,classificationOutputVector[j]);
            rtlogger.logInfo(message);
        }
    }

    /** TRIGGER OUTPUT NOTE WHEN CONDITIONS ARE MET **/
    if(prediction != 4) // Add (confidence > 0.90) if useful
        sinewt.playNote( pow(440,prediction) ); // Play A4 through A7 depending on percussive timbre
}

void DemoProcessor::computeFeatureVector()
{
    /*- Current best feature vector composition ------------------/
    |                                                             |
    |   110 Coefficients:                                         |
    |    - 50 BFCC                                                |
    |    - First 60 Cepstrum coefficients                         |
    |                                                             |
    /------------------------------------------------------------*/

    /*-------------------------------------/
    | Bark Frequency Cepstral Coefficients |
    /-------------------------------------*/
    bfccRes = this->bfcc.compute();
    if (bfccRes.size() != BFCC_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for bfcc");
    for(int i=0; i<BFCC_RES_SIZE; ++i)
        this->featureVector[i] = bfccRes[i];


    /*-------------------------------------/
    | Cepstrum Coefficients                |
    /-------------------------------------*/
    cepstrumRes = this->cepstrum.compute();
    if (cepstrumRes.size() != CEPSTRUM_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for Cepstrum");
    /* Copy only the first 60 Cespstrum coefficients */
    for(int i=0; i<60; ++i)
        this->featureVector[50 + i] = cepstrumRes[i];

    //
    //-----------------------------------------------------------------------------------
    //

    /********************************************/
    /* Other Feature extractors                 */
    /* can be used. The commands to perfom      */
    /* computation are shown here:              */
    /* Note: remember to call prepare and       */
    /* release methods too                      */
    /********************************************/

    /*------------------------------------/
    | Attack time                         |
    /------------------------------------*/
    // attackTime.compute(...);

    /*------------------------------------/
    | Bark Spectral Brightness            |
    /------------------------------------*/
    // barkSpecBrightness.compute();

    /*------------------------------------/
    | Bark Spectrum                       |
    /------------------------------------*/
    // barkSpec.compute();

    /*------------------------------------/
    | Mel Frequency Cepstral Coefficients |
    /------------------------------------*/
    // mfcc.compute();

    /*------------------------------------/
    | Peak sample                         |
    /------------------------------------*/
    // peakSample.compute();

    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    // zeroCrossing.compute();

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
