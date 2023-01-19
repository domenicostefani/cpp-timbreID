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

#ifdef LONG_WINDOW
 #define POST_ONSET_DELAY_MS 92.2933333333 // This delay is infact then rounded to the closest multiple of the audio block period
                                           // In the case of 92.2933333333ms at 48000Hz and 64 samples blocksizes, the closes delay is 
                                           // 92.0ms, corresponding to 69 audio block periods
#else
 #define POST_ONSET_DELAY_MS 6.96 // This delay is infact then rounded to the closest multiple of the audio block period
                                 // In the case of 6.96ms at 48000Hz and 64 samples blocksizes, the closes delay is 
                                 // 6.66ms, corresponding to 5 audio block periods
#endif

#define DO_DELAY_ONSET // If not defined there is NO delay between onset detection and feature extraction

WFE::FeatureExtractors<DEFINED_WINDOW_SIZE,
                      DO_USE_ATTACKTIME,
                      DO_USE_BARKSPECBRIGHTNESS,
                      DO_USE_BARKSPEC,
                      DO_USE_BFCC,
                      DO_USE_CEPSTRUM,
                      DO_USE_MFCC,
                      DO_USE_PEAKSAMPLE,
                      DO_USE_ZEROCROSSING,
                      BLOCK_SIZE,
                      FRAME_SIZE,
                      FRAME_INTERVAL ,
                      ZEROPADS> DemoProcessor::featexts;


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
                       ),
#endif
    parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{    
    suspendProcessing (true);
    header = featexts.getHeader();
    rtlogger.logInfo("Initializing Onset detector");

    /** ADD ONSET DETECTOR LISTENER **/
    aubioOnset.addListener(this);

   #ifdef DO_LOG_TO_FILE
    /** START POLLING ROUTINE THAT WRITES TO FILE ALL THE LOG ENTRIES **/
    pollingTimer.startLogRoutine(100); // Call every 0.1 seconds
   #endif


    parameters.state = ValueTree("savedParams");
    parameters.addParameterListener(STORESTATE_ID,this);
    parameters.addParameterListener(CLEAR_ID,this);
    parameters.addParameterListener(SAVEFILE_ID,this);

    // for (auto p : getParameters())
    // {
    //     if (auto param = dynamic_cast<AudioProcessorParameterWithID*> (p))
    //         parameterChanged (param->paramID, *parameters.getRawParameterValue (param->paramID));
    // }

    rtlogger.logValue("HasEditor",this->hasEditor());

    suspendProcessing (false);
}

void DemoProcessor::logInCsvSpecial(std::string messagestr)
{
    Entry<VECTOR_SIZE>* currentEntry = csvsaver.entryToWrite();

    if (!currentEntry)
        rtlogger.logInfo("ERROR: csv buffer full");
    else
    {
        currentEntry->onsetDetectionTime = sampleCounter; // juce::Time::getMillisecondCounterHiRes();
        currentEntry->featureComputationTime = sampleCounter; // juce::Time::getMillisecondCounterHiRes();
        currentEntry->featureExtractionWindowSize = DEFINED_WINDOW_SIZE;
        currentEntry->sampleRate = this->pluginSampleRate;
        currentEntry->blockSize = this->pluginBlockSize;
        currentEntry->writeMessage(messagestr.c_str());
        /*--------------------/
        | 3. STORE Entry      |
        /--------------------*/
        size_t oc = csvsaver.confirmEntry();
        // onsetCounterAtomic.exchange(oc);
    }
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

    logInCsvSpecial("StartProcessing");

    this->pluginSampleRate = (int)sampleRate;
    this->pluginBlockSize = samplesPerBlock;

    /** INIT ONSET DETECTOR MODULE**/
    if (DISABLE_ADAPTIVE_WHITENING)
        aubioOnset.setAdaptiveWhitening(false);
    aubioOnset.prepare(sampleRate,samplesPerBlock);

    /** INIT POST ONSET TIMER **/
   #ifdef DO_DELAY_ONSET
    postOnsetTimer.prepare(sampleRate,samplesPerBlock);
   #endif

    featexts.prepare(sampleRate,samplesPerBlock);

    this->sampleRate = sampleRate;
    this->samplesPerBlock = samplesPerBlock;

    rtlogger.logInfo("+--Prepare to play completed");
}

void DemoProcessor::releaseResources()
{
    rtlogger.logInfo("+--ReleaseResources called");
    rtlogger.logInfo("+  Releasing Onset detector resources");

    logInCsvSpecial("StopProcessing");

    /** FREE ONSET DETECTOR MEMORY **/
    aubioOnset.reset();

    rtlogger.logInfo("+  Releasing extractor resources");

    /*------------------------------------/
    | Reset the feature extractors        |
    /------------------------------------*/
    featexts.reset();

    rtlogger.logInfo("+--ReleaseResources completed");
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    sampleCounter += buffer.getNumSamples();
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
    featexts.storeAndCompute(buffer,(short int)MONO_CHANNEL);

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
    buffer.clear(); // Uncomment to let input pass through

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
    // this->onsetDetectionTime =  Time::Time::getHighResolutionTicks();
    if(aubioOnset == &this->aubioOnset)
    {
        this->onsetMonitorState.exchange(true);
        this->lastOnsetDetectionTime = sampleCounter; //juce::Time::getMillisecondCounterHiRes();
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
            this->featexts.computeFeatureVectors(currentFV->features);

            /*--------------------/
            | 2. ADD METADATA     |
            /--------------------*/
            currentFV->onsetDetectionTime = this->lastOnsetDetectionTime; //double
            currentFV->featureComputationTime = sampleCounter; //juce::Time::getMillisecondCounterHiRes(); //double
            currentFV->featureExtractionWindowSize = DEFINED_WINDOW_SIZE;   // int
            currentFV->sampleRate = this->pluginSampleRate;   // int
            currentFV->blockSize = this->pluginBlockSize;    // int
            /*--------------------/
            | 3. STORE FEATURES   |
            /--------------------*/
            size_t oc = csvsaver.confirmEntry();

            onsetCounterAtomic++;   // I stopped using the next line when introducing special log entries in the csv buffer, otherwise they would be counted.
            // onsetCounterAtomic.exchange(oc);
        }
    }
    else
    {
        rtlogger.logInfo("Extractor idling");
    }
}

void DemoProcessor::parameterChanged(const juce::String & parameterID, float newValue)
{
    // std::cout << parameterID << " changed (" << newValue << ")\n" << std::flush;
    if (parameterID == this->STORESTATE_ID)
        this->storageState = (int)(newValue == 0) ? StorageState::store : StorageState::idle;
    else if (parameterID == this->CLEAR_ID)
    {
        this->clear = true;
        // std::cout << "set clear to " << std::endl;
        // std::cout << "clearAll pressed" << std::endl;
        // this->processor.csvsaver.clearEntries();
        // this->processor.onsetCounterAtomic.store(0);
    }    
    else if (parameterID == this->SAVEFILE_ID)
    {
        // std::cout << "write pressed" << std::endl;
        this->savefile = true;
        // File temp = File(write.getText1()+write.getText2());
        // std::string path = temp.getFullPathName().toStdString();
        // std::cout << "path " << path << std::endl;

        // this->processor.csvsaver.writeToFile(path,processor.header);
        // write.setDefaultText2(generateName(this->dirpath));
    }
}

/** Create the parameters to add to the value tree state
 * In this case only the boolean recording state (true = rec, false = stop)
*/
AudioProcessorValueTreeState::ParameterLayout DemoProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<RangedAudioParameter>> parameters;
    parameters.push_back(std::make_unique<AudioParameterBool>(STORESTATE_ID, STORESTATE_NAME,JUCEApplicationBase::isStandaloneApp() ? false : true));
    parameters.push_back(std::make_unique<AudioParameterBool>(CLEAR_ID, CLEAR_NAME,false));
    parameters.push_back(std::make_unique<AudioParameterBool>(SAVEFILE_ID, SAVEFILE_NAME,false));

    return {parameters.begin(), parameters.end()};
}

//== SAVING PARAMETERS =========================================================
void DemoProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // Save parameters
    std::unique_ptr<XmlElement> xml(parameters.state.createXml());
    copyXmlToBinary(*xml,destData);
}

void DemoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Restore parameters
    std::unique_ptr<XmlElement> xml(getXmlFromBinary(data,sizeInBytes));

    if(xml != nullptr)
        if(xml->hasTagName(parameters.state.getType()))
            parameters.state = juce::ValueTree::fromXml(*xml);

    // for (auto p : getParameters())
    // {
    //     if (auto param = dynamic_cast<AudioProcessorParameterWithID*> (p))
    //         parameterChanged (param->paramID, *parameters.getRawParameterValue (param->paramID));
    // }
    this->storageState = StorageState::idle;
    
}


/*    JUCE stuff ahead, not relevant to the demo    */







// Change Juce plugin name and prepend "superlong" if LONG_WINDOW is defined
#ifdef LONG_WINDOW
 #define Plugin_final_name "superlong"
#else
 #define Plugin_final_name JucePlugin_Name
#endif



//==============================================================================
const String DemoProcessor::getName() const{return Plugin_final_name;}
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

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemoProcessor();
}
