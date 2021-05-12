/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - Feature Extractor Plugin

  Extractors
  attackTime
  barkSpecBrightness
  barkSpec
  bfcc
  cepstrum
  mfcc
  peakSample
  zeroCrossing

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 10th September 2020

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define MONO_CHANNEL 0
#define POST_ONSET_DELAY_MS 6.96 // This delay is infact then rounded to the closest multiple of the audio block period
                                 // In the case of 6.96ms at 48000Hz and 64 samples blocksizes, the closes delay is 
                                 // 6.66ms, corresponding to 5 audio block periods
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

    /** ADD ONSET DETECTOR LISTENER **/
    aubioOnset.addListener(this);

    barkSpecRes.resize(BARKSPEC_SIZE);
    bfccRes.resize(BFCC_RES_SIZE);
    cepstrumRes.resize(CEPSTRUM_RES_SIZE);
    mfccRes.resize(MFCC_RES_SIZE);

    attackTime.setMaxSearchRange(20);
   #ifdef DO_LOG_TO_FILE
    /** START POLLING ROUTINE THAT WRITES TO FILE ALL THE LOG ENTRIES **/
    pollingTimer.startLogRoutine(100); // Call every 0.1 seconds
   #endif
}

DemoProcessor::~DemoProcessor(){
   #ifdef DO_LOG_TO_FILE
    /** Log last queued messages before closing **/
    logPollingRoutine();
   #endif
}

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    rtlogger.logInfo("+--Prepare to play called");
    rtlogger.logInfo("+  Setting up onset detector");

    /** INIT ONSET DETECTOR MODULE**/
    if (DISABLE_ADAPTIVE_WHITENING)
        aubioOnset.setAdaptiveWhitening(false);
    aubioOnset.prepare(sampleRate,samplesPerBlock);

    /** INIT POST ONSET TIMER **/
   #ifdef DO_DELAY_ONSET
    postOnsetTimer.prepare(sampleRate,samplesPerBlock);
   #endif

    /** Prepare feature extractors **/

    /*-------------------------------------/
    | Bark Frequency Cepstral Coefficients |
    /-------------------------------------*/
    bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Cepstrum Coefficients               |
    /------------------------------------*/
    cepstrum.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Attack time                         |
    /------------------------------------*/
    attackTime.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Bark spectral brightness            |
    /------------------------------------*/
    barkSpecBrightness.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Bark Spectrum                       |
    /------------------------------------*/
    barkSpec.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    mfcc.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    peakSample.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    zeroCrossing.prepare(sampleRate, (uint32)samplesPerBlock);

    rtlogger.logInfo("+--Prepare to play completed");
}

void DemoProcessor::releaseResources()
{
    rtlogger.logInfo("+--ReleaseResources called");
    rtlogger.logInfo("+  Releasing Onset detector resources");

    /** FREE ONSET DETECTOR MEMORY **/
    aubioOnset.reset();

    rtlogger.logInfo("+  Releasing extractor resources");

    /*------------------------------------/
    | Reset the feature extractors        |
    /------------------------------------*/
    bfcc.reset();
    cepstrum.reset();
    attackTime.reset();
    barkSpecBrightness.reset();
    barkSpec.reset();
    mfcc.reset();
    peakSample.reset();
    zeroCrossing.reset();

    rtlogger.logInfo("+--ReleaseResources completed");
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
   #ifdef LOG_LATENCY
    int64 timeAtProcessStart = 0;
    if (++latencyCounter >= latencyLogPeriod)
    {
        timeAtProcessStart = Time::Time::getHighResolutionTicks();
        latencyCounter = 0;
    }
    int64 computeRoundStart = 0;
   #endif
    /** EXTRACT FEATURES AND CLASSIFY IF POST ONSET TIMER IS EXPIRED **/
    if(postOnsetTimer.isExpired())
    {
       #ifdef LOG_LATENCY
        computeRoundStart = Time::Time::getHighResolutionTicks();
       #endif
        onsetDetectedRoutine();
    }

    /** STORE THE BUFFER FOR FEATURE EXTRACTION **/
    bfcc.store(buffer,(short int)MONO_CHANNEL);
    cepstrum.store(buffer,(short int)MONO_CHANNEL);
    attackTime.store(buffer,(short int)MONO_CHANNEL);
    barkSpecBrightness.store(buffer,(short int)MONO_CHANNEL);
    barkSpec.store(buffer,(short int)MONO_CHANNEL);
    mfcc.store(buffer,(short int)MONO_CHANNEL);
    peakSample.store(buffer,(short int)MONO_CHANNEL);
    zeroCrossing.store(buffer,(short int)MONO_CHANNEL);

    /** STORE THE ONSET DETECTOR BUFFER **/
    try
    {
        aubioOnset.store(buffer,MONO_CHANNEL);
    } catch(std::exception& e) {
        rtlogger.logInfo("An exception has occurred while buffering: ",e.what());
    } catch(...) {
        rtlogger.logInfo("An unknwn exception has occurred while buffering: ");
    }
    /** CLEAR THE BUFFER (OPTIONAL) **/
    // buffer.clear(); // Uncomment to let input pass through

    /** UPDATE POST ONSET COUNTER/TIMER **/
   #ifdef DO_DELAY_ONSET
    postOnsetTimer.updateTimer(); // Update the block counter of the postOnsetTimer
   #endif

   #ifdef LOG_LATENCY
    if (timeAtProcessStart)
    {
        int64 timeAtProcessEnd = Time::Time::getHighResolutionTicks();
        rtlogger.logInfo("ProcessedBlock",timeAtProcessStart,timeAtProcessEnd);
    }
    if (computeRoundStart)
    {
        int64 computeRoundFinish = Time::Time::getHighResolutionTicks();
        rtlogger.logInfo("Compute Round",computeRoundStart,computeRoundFinish);
    }
   #endif
}

void DemoProcessor::onsetDetected (tid::aubio::Onset<float> *aubioOnset){
    if(aubioOnset == &this->aubioOnset)
    {
        this->onsetMonitorState.exchange(true);
       #ifdef DO_DELAY_ONSET
        if(postOnsetTimer.isIdle())
        {
            float actualDelayMs = postOnsetTimer.start(POST_ONSET_DELAY_MS);
            rtlogger.logValue("Start waiting for ",actualDelayMs,"ms");
            rtlogger.logValue("(Closes approximation to ",(float)POST_ONSET_DELAY_MS,"ms in block sizes)");
        }
       #else
        onsetDetectedRoutine();
       #endif
    }
}

/**
 * Onset Detected Callback
 *
**/
void DemoProcessor::onsetDetectedRoutine ()
{
    bool VERBOSERES = true; // Log info about classification result (Suggested: true)
    bool VERBOSE = false;   // Log general verbose info (Suggested: false)

    rtlogger.logInfo("Onset detected");
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];

    Entry<VECTOR_SIZE>* currentFV = csvsaver.entryToWrite();

    if (storageState.load() == StorageState::store)
    {
        if (!currentFV)
            rtlogger.logInfo("ERROR: buffer full");
        else
        {
            /*--------------------/
            | 1. EXTRACT FEATURES |
            /--------------------*/
            this->computeFeatureVector(currentFV->features);

            /*-------------------/
            | 2. STORE FEATURES  |
            /-------------------*/
            size_t oc = csvsaver.confirmEntry();
            onsetCounterAtomic.exchange(oc);
        }
    }
    else
    {
        rtlogger.logInfo("Extractor idling");
    }
}

void DemoProcessor::computeFeatureVector(float featureVector[])
{
    int last = -1;
    int newLast = 0;
    /*-----------------------------------------/
    | 01 - Attack time                         |
    /-----------------------------------------*/
    unsigned long int peakSampIdx = 0;
    unsigned long int attackStartIdx = 0;
    float attackTimeValue = 0.0f;
    this->attackTime.compute(&peakSampIdx, &attackStartIdx, &attackTimeValue);

    featureVector[0] = (float)peakSampIdx;
    featureVector[1] = (float)attackStartIdx;
    featureVector[2] = attackTimeValue;
    newLast = 2;
   #ifdef LOG_SIZES
    info += ("attackTime [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
   #endif
    last = newLast;

    /*-----------------------------------------/
    | 02 - Bark Spectral Brightness            |
    /-----------------------------------------*/
    float bsb = this->barkSpecBrightness.compute();

    featureVector[3] = bsb;
    newLast = 3;
   #ifdef LOG_SIZES
    info += ("barkSpecBrightness [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
   #endif
    last = newLast;

    /*-----------------------------------------/
    | 03 - Bark Spectrum                       |
    /-----------------------------------------*/
    barkSpecRes = this->barkSpec.compute();

    jassert(barkSpecRes.size() == BARKSPEC_SIZE);
    for(int i=0; i<BARKSPEC_SIZE; ++i)
    {
        featureVector[(last+1) + i] = barkSpecRes[i];
    }
    newLast = last + BARKSPEC_SIZE;
   #ifdef LOG_SIZES
    info += ("barkSpec [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
   #endif
    last = newLast;

    /*------------------------------------------/
    | 04 - Bark Frequency Cepstral Coefficients |
    /------------------------------------------*/
    bfccRes = this->bfcc.compute();
    jassert(bfccRes.size() == BFCC_RES_SIZE);
    for(int i=0; i<BFCC_RES_SIZE; ++i)
    {
        featureVector[(last+1) + i] = bfccRes[i];
    }
    newLast = last + BFCC_RES_SIZE;
   #ifdef LOG_SIZES
    info += ("bfcc [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
   #endif
    last = newLast;

    /*------------------------------------------/
    | 05 - Cepstrum Coefficients                |
    /------------------------------------------*/
    cepstrumRes = this->cepstrum.compute();
    jassert(cepstrumRes.size() == CEPSTRUM_RES_SIZE);
    for(int i=0; i<CEPSTRUM_RES_SIZE; ++i)
    {
        featureVector[(last+1) + i] = cepstrumRes[i];
    }
    newLast = last + CEPSTRUM_RES_SIZE;
   #ifdef LOG_SIZES
    info += ("cepstrum [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
   #endif
    last = newLast;

    /*-----------------------------------------/
    | 06 - Mel Frequency Cepstral Coefficients |
    /-----------------------------------------*/
    mfccRes = this->mfcc.compute();
    jassert(mfccRes.size() == MFCC_RES_SIZE);
    for(int i=0; i<MFCC_RES_SIZE; ++i)
    {
        featureVector[(last+1) + i] = mfccRes[i];
    }
    newLast = last + MFCC_RES_SIZE;
   #ifdef LOG_SIZES
    info += ("mfcc [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
   #endif
    last = newLast;

    /*-----------------------------------------/
    | 07 - Peak sample                         |
    /-----------------------------------------*/
    std::pair<float, unsigned long int> peakSample = this->peakSample.compute();
    float peakSampleRes = peakSample.first;
    unsigned long int peakSampleIndex = peakSample.second;
    featureVector[last+1] = peakSampleRes;
    featureVector[last+2] = peakSampleIndex;
    newLast = last + 2;
   #ifdef LOG_SIZES
    info += ("peakSample [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
   #endif
    last = newLast;

    /*-----------------------------------------/
    | 08 - Zero Crossings                      |
    /-----------------------------------------*/
    uint32 crossings = this->zeroCrossing.compute();
    featureVector[last+1] = crossings;
    newLast = last +1;
   #ifdef LOG_SIZES
    info += ("zeroCrossing [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
   #endif
    last = newLast;

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
