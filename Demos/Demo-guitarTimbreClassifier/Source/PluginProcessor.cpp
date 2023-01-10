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

#include "readJsonConfig.h"

#include <vector>
#include <chrono>

#define MONO_CHANNEL 0
#define POST_ONSET_DELAY_MS 6.96 // This delay is infact then rounded to the closest multiple of the audio block period
                                 // In the case of 6.96ms at 48000Hz and 64 samples blocksizes, the closes delay is 
                                 // 6.66ms, corresponding to 5 audio block periods
#define DO_DELAY_ONSET // If not defined there is NO delay between onset detection and feature extraction

#ifndef SEQUENTIAL_CLASSIFICATION
 CData::ClassificationData DemoProcessor::classificationData;
#endif
ClassifierPtr DemoProcessor::timbreClassifier = nullptr;



//==============================================================================
DemoProcessor::DemoProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::mono(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
#ifndef SEQUENTIAL_CLASSIFICATION
    ,
    classificationThread(&DemoProcessor::classificationData,&DemoProcessor::timbreClassifier)
#endif
{
    suspendProcessing (true);
  #ifndef FAST_MODE_1
   rtlogger.logInfo("Initializing Onset detector");
   pollingTimer.startLogRoutine(100); // Call every 0.1 seconds
  #endif

   #ifndef SEQUENTIAL_CLASSIFICATION
    rtlogger.logInfo("Running classification in a SEPARATE THREAD (Not rt thread)");
   #else
    rtlogger.logInfo("Running classification SEQUENTIALLY in the real-time thread");
   #endif

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

    /** ALLOCATE MEMORY FOR TMP VECTORS **/
    barkSpecRes.resize(BARKSPEC_RES_SIZE);
    bfccRes.resize(BFCC_RES_SIZE);
    cepstrumRes.resize(CEPSTRUM_RES_SIZE);
    mfccRes.resize(MFCC_RES_SIZE);

    juce::var parsedJson;
    try
    {
        parsedJson = JsonConf::parseJson(NN_CONFIG_JSON_PATH);
       #if defined(USE_TFLITE)
        rtlogger.logInfo("Neural Network interpreter: Tensorflow Lite");
        MODEL_PATH = JsonConf::getNestedStringProperty(parsedJson,"models","tensorflowlite");
       #elif defined(USE_RTNEURAL_CTIME)
        rtlogger.logInfo("Neural Network interpreter: Rt neural, Compile-time defined model");
        MODEL_PATH = JsonConf::getNestedStringProperty(parsedJson,"models","rtneural");
       #elif defined(USE_RTNEURAL_RTIME)
        rtlogger.logInfo("Neural Network interpreter: Rt neural, Run-time interpreted model");
        MODEL_PATH = JsonConf::getNestedStringProperty(parsedJson,"models","rtneural");
       #elif defined(USE_ONNX)
        rtlogger.logInfo("Neural Network interpreter: OnnxRuntime");
        MODEL_PATH = JsonConf::getNestedStringProperty(parsedJson,"models","onnx");
       #elif USE_TORCHSCRIPT
        rtlogger.logInfo("Neural Network interpreter: TorchScript");
        MODEL_PATH = JsonConf::getNestedStringProperty(parsedJson,"models","torchscript");
       #else
        rtlogger.logInfo("ERROR, can't find a preprocessor directive with the neural interpreter to use");
       #endif

        if (!juce::File(MODEL_PATH).existsAsFile())
            throw std::logic_error("Model file does not exist at specified path ("+MODEL_PATH+")");

       #ifndef STOP_OSC
        std::string enableOSC = JsonConf::getNestedStringProperty(parsedJson,"osc","enable");
        if ((enableOSC == "true") || (enableOSC == "True") || (enableOSC == "TRUE"))
        {
            std::string oscIP = JsonConf::getNestedStringProperty(parsedJson,"osc","target-ip");
            std::cout << "Setting OSC ip to " << oscIP << std::endl << std::flush;
            int oscPort = std::stoi(JsonConf::getNestedStringProperty(parsedJson,"osc","target-port"));
            std::cout << "Setting OSC port to " << oscPort << std::endl << std::flush;
            this->classificationThread.connectOSC(oscIP,oscPort);

            
            std::string oscMsg = JsonConf::getNestedStringProperty(parsedJson,"osc","osc-address");
            std::cout << "Setting OSC message address to " << oscMsg << std::endl << std::flush;
            this->classificationThread.setOSCmessage(oscMsg);
            
            std::string classMsgType = JsonConf::getNestedStringProperty(parsedJson,"osc","class-msg-type");
            if (classMsgType == "all")
            {
                std::cout << "Setting OSC message type to send the entire output layer values (8)" << std::endl << std::flush;
                this->classificationThread.setFullOSCmessage(true);
            } 
            else if (classMsgType == "best")
            {
                std::cout << "Setting OSC message type to send a single integer with the predicted class (best score)" << std::endl << std::flush;
                this->classificationThread.setFullOSCmessage(false);
            }
            else
            {
                std::cerr << "Invalid message type in JSON config (use either 'all' or 'best')" << std::endl << std::flush;
            }
           #ifndef SEQUENTIAL_CLASSIFICATION
            //TODO: Check out this and what happens sequentially
            this->classificationThread.setOSCmessage(oscMsg);
           #endif
        }
       #endif
       
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << std::endl << std::flush;
        rtlogger.logInfo(e.what());
    }
    rtlogger.logValue("Model Path: ",MODEL_PATH.c_str());


    /** SET UP FEATURE COMPUTATION **/
    std::vector<std::string> selectedFeatures = JsonConf::getNestedStringVectorProperty(parsedJson, "features","selected_features");    // Read JSON
    featParser = std::make_unique<JsonConf::FeatureParser>(selectedFeatures);   // Prepare list
    // Log everything
    std::string precList = "[ ";
    for (const JsonConf::feature_extractor_t item : featParser->precomputationList)
        precList += JsonConf::stringPrefixes.at(item) + " ";
    precList += "]";
    rtlogger.logValue("Feature Extraction configuration read from file: ",precList.c_str());

    rtlogger.logInfo("Feature List: ");
    for (const JsonConf::FeatureParser::Feature* item : featParser->featureList) {
        const JsonConf::FeatureParser::VectorFeature* vectorFeature = dynamic_cast<const JsonConf::FeatureParser::VectorFeature*>((JsonConf::FeatureParser::Feature*)item);
        const JsonConf::FeatureParser::NamedFeature* namedFeature = dynamic_cast<const JsonConf::FeatureParser::NamedFeature*>((JsonConf::FeatureParser::Feature*)item);
        if (vectorFeature != nullptr)
            rtlogger.logInfo((JsonConf::stringPrefixes.at(vectorFeature->extractorType) + "[" + std::to_string(vectorFeature->index) + "] ").c_str());
        else if (namedFeature != nullptr)
            rtlogger.logInfo(((JsonConf::stringPrefixes.at(namedFeature->extractorType)) + ("->") + (namedFeature->valuename) + " ").c_str());
        else 
            rtlogger.logInfo(((JsonConf::stringPrefixes.at(item->extractorType)) + " ").c_str());
    }
    // !! VERY IMPORTANT !! resize the feature vector to the target size, otherwise it will not work with [] indexing. Push_back or other operation that will cause dyamic allocation cannot be used.
    this->featureVector.resize(featParser->featureList.size());
   #ifndef SEQUENTIAL_CLASSIFICATION
    this->classificationThread.setFeatureNumber(featParser->featureList.size());
   #endif



    /** SET UP CLASSIFIER **/
   #ifndef FAST_MODE_1
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];
    snprintf(message,sizeof(message),"Model path set to '%s'",MODEL_PATH.c_str());
    rtlogger.logInfo(message);   
    double modelLoadTime = juce::Time::getMillisecondCounterHiRes();
   #endif
    DemoProcessor::timbreClassifier = createClassifier(MODEL_PATH);
   #ifndef FAST_MODE_1
    rtlogger.logValue("Classifier object instantiated at time:  ",juce::Time::getMillisecondCounterHiRes());
    rtlogger.logValue("(Classifier object created in ",(juce::Time::getMillisecondCounterHiRes()-modelLoadTime),"ms");
   #endif

    suspendProcessing (false);
}

DemoProcessor::~DemoProcessor(){
    // /** Free classifier memory **/
    deleteClassifier(DemoProcessor::timbreClassifier);
   #ifndef FAST_MODE_1
    rtlogger.logInfo("Classifier object deleted");
    /** Log last queued messages before closing **/
    logPollingRoutine();
   #endif
}

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
   #ifndef FAST_MODE_1
    rtlogger.logInfo("+--Prepare to play called");
    rtlogger.logInfo("+  Setting up onset detector");
   #endif

    /** INIT ONSET DETECTOR MODULE**/
   #ifdef USE_AUBIO_ONSET
    if (DISABLE_ADAPTIVE_WHITENING)
        aubioOnset.setAdaptiveWhitening(false);
    aubioOnset.prepare(sampleRate,samplesPerBlock);
   #else
    bark.prepare(sampleRate, (uint32)samplesPerBlock);
   #endif

    /** INIT POST ONSET TIMER **/
   #ifdef DO_DELAY_ONSET
    postOnsetTimer.prepare(sampleRate,samplesPerBlock);
   #endif

    /** PREPARE FEATURE EXTRACTORS **/

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
    | Bark Frequency Cepstral Coefficients|
    /------------------------------------*/
    bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
    /*------------------------------------/
    | Cepstrum Coefficients               |
    /------------------------------------*/
    cepstrum.prepare(sampleRate, (uint32)samplesPerBlock);
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

    sinewt.prepareToPlay(sampleRate);
    squaredsinewt.prepareToPlay(sampleRate);   

   #ifndef FAST_MODE_1
    rtlogger.logInfo("+--Prepare to play completed");
   #endif

    // this->computeFeatureVector();
    // classify(DemoProcessor::timbreClassifier,&(featureVector[0]), featureVector.size(),&(classificationOutputVector[0]), classificationOutputVector.size());
}

void DemoProcessor::releaseResources()
{
   #ifndef FAST_MODE_1
    rtlogger.logInfo("+--ReleaseResources called");
    rtlogger.logInfo("+  Releasing Onset detector resources");
   #endif

    /** FREE ONSET DETECTOR MEMORY **/
   #ifdef USE_AUBIO_ONSET
    aubioOnset.reset();
   #else
    bark.reset();
   #endif
   #ifndef FAST_MODE_1
    rtlogger.logInfo("+  Releasing extractor resources");
   #endif

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

   #ifndef FAST_MODE_1
    rtlogger.logInfo("+--ReleaseResources completed");
   #endif
}

/**
 * @brief Add spike to the beginning of the current audio buffer
 * 
 * This function adds a spike to the beginning of the audio signal.
 * The parameter determine the value of the sample to use (-1.0,+1.0)
 * and its length (1,Buffer Size).
 * It is used to highlight specific instants in the output signal,
 * so to be able to measure a posteriori the time intervales between 
 * events (e.g., classification latency). 
 * 
 * @param buffer        The audio buffer to modify
 * @param spikeLength   The length of the spike 
 * @param spikeHeight   The value of the samples in the spike
 */
void sendSpike(AudioBuffer<float>& buffer, unsigned int spikeLength, float spikeHeight) {

    // Clamp spike height (-1.0,+1.0)
    spikeHeight = spikeHeight > 1.0 ? 1.0 : spikeHeight;
    spikeHeight = spikeHeight < -1.0 ? -1.0 : spikeHeight;

    // Clamp spike length (1,Buffer Size)
    spikeLength = spikeLength > buffer.getNumSamples() ? buffer.getNumSamples() : spikeLength;
    spikeLength = spikeLength < 1 ? 1 : spikeLength;

    for (int sample = 0; sample < spikeLength; ++sample) {
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel) {
            float* const bufferPtr = buffer.getWritePointer(channel, 0);
            bufferPtr[sample] = spikeHeight;
        }
    }
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    if (this->primeClassifier)
    {
        primeClassifier = false;
        this->computeFeatureVector();
        classify(DemoProcessor::timbreClassifier,&(featureVector[0]), featureVector.size(),&(classificationOutputVector[0]), classificationOutputVector.size());
    }


    ScopedNoDenormals noDenormals;
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH]; // logger message
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    /** LOG THE DIFFERENT TIMESTAMPS FOR LATENCY COMPUTATION **/
  #ifndef FAST_MODE_1
   #ifdef MEASURE_COMPUTATION_LATENCY
    int64 timeAtBlockStart = 0;
    if (++logProcessblock_counter >= logProcessblock_period)
    {
        logProcessblock_fixedCounter += logProcessblock_counter; // Add the number of processed blocks to the total
        logProcessblock_counter = 0;
        timeAtBlockStart = Time::Time::getHighResolutionTicks();
    }
   #endif
  #endif

    /** EXTRACT FEATURES AND CLASSIFY IF POST ONSET TIMER IS EXPIRED **/
    if(postOnsetTimer.isExpired())
        onsetDetectedRoutine();

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
       #ifdef USE_AUBIO_ONSET
        aubioOnset.store(buffer,MONO_CHANNEL);
       #else
        /** STORE THE BUFFER FOR COMPUTATION **/
        bark.store(buffer,(short int)MONO_CHANNEL);
       #endif
    } catch(std::exception& e) {
       #ifndef FAST_MODE_1
        rtlogger.logInfo("An exception has occurred while buffering: ",e.what());
       #endif
    } catch(...) {
       #ifndef FAST_MODE_1
        rtlogger.logInfo("An unknwn exception has occurred while buffering");
       #endif
    }
    /** CLEAR THE BUFFER (OPTIONAL) **/
    // buffer.clear(); // Comment to let input pass through

   #ifndef SEQUENTIAL_CLASSIFICATION
    /** CHECK IF THE CLASSIFIER HAS FINIHED **/
    bool readPred = DemoProcessor::classificationData.predictionBuffer.read(classificationOutputVector); // read predictions from rt thread when available
    if (readPred) // if a value was read
    {
        classificationFinished();
    }
   #endif


   #ifdef DEBUG_WITH_SPIKE
    squaredsinewt.processBlock(buffer,1);
   #else
    /** ADD OUTPUT SOUND (TRIGGERED BY TIMBRE CLASSIFICATION) **/
    sinewt.processBlock(buffer);
   #endif

    /** UPDATE POST ONSET COUNTER/TIMER **/
   #ifdef DO_DELAY_ONSET
    postOnsetTimer.updateTimer(); // Update the block counter of the postOnsetTimer
   #endif

  #ifndef FAST_MODE_1
    /** lOG LAST TIMESTAMPS FOR LATENCY COMPUTATION **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    if (timeAtBlockStart)   // If timeAtBlockStart was initialized this round, we are computing and logging processBlock Duration
    {
        snprintf(message,sizeof(message),"block nr %lld processed",logProcessblock_fixedCounter);
        rtlogger.logInfo(message,timeAtBlockStart,Time::Time::getHighResolutionTicks());
        timeAtBlockStart = 0;
    }
   #endif
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
      #ifndef FAST_MODE_1
       #ifdef MEASURE_COMPUTATION_LATENCY
        latencyTime = juce::Time::getMillisecondCounterHiRes();
       #endif
      #endif

       #ifdef DO_DELAY_ONSET
        if(postOnsetTimer.isIdle())
        {
            float actualDelayMs = postOnsetTimer.start(POST_ONSET_DELAY_MS);
           #ifndef FAST_MODE_1
            rtlogger.logValue("Start waiting for ",actualDelayMs,"ms");
            rtlogger.logValue("(Closes approximation to ",(float)POST_ONSET_DELAY_MS,"ms in block sizes)");
           #endif
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

    
  #ifndef FAST_MODE_1
    rtlogger.logInfo("Onset detected");
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];

    /** LOG BEGINNING OF FEATURE EXTRACTION **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    rtlogger.logValue("Feature extraction started at ",juce::Time::getMillisecondCounterHiRes());
    rtlogger.logValue("(",(juce::Time::getMillisecondCounterHiRes() - latencyTime),"ms after onset detection)");
   #endif
  #endif

    /*--------------------/
    | 1. EXTRACT FEATURES |
    /--------------------*/
    try //TODO: remove
    { //TODO: remove
        this->computeFeatureVector();
    } //TODO: remove
    catch(const std::exception& e) //TODO: remove
    { //TODO: remove
        std::cerr << e.what() << '\n'; //TODO: remove
    } //TODO: remove
    

  #ifndef FAST_MODE_1
    /** LOG ENDING OF FEATURE EXTRACTION **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    rtlogger.logValue("Feature extraction stopped at ",juce::Time::getMillisecondCounterHiRes());
    rtlogger.logValue("(Feature extraction stopped ",(juce::Time::getMillisecondCounterHiRes() - latencyTime),"ms after onset detection)");
   #endif
  #endif

    /*----------------------------------/
    | 2. TRIGGER TIMBRE CLASSIFICATION  |
    /----------------------------------*/
  #ifndef FAST_MODE_1
   #ifdef MEASURE_COMPUTATION_LATENCY
    rtlogger.logValue("(",(juce::Time::getMillisecondCounterHiRes()-latencyTime),"ms after onset detection");
    this->classf_start = juce::Time::getMillisecondCounterHiRes();
   #endif
  #endif

   
    this->chrono_start = std::chrono::high_resolution_clock::now();
   #ifndef SEQUENTIAL_CLASSIFICATION
    DemoProcessor::classificationData.featureBuffer.write(featureVector);    // Write the feature vector to a ring buffer, for parallel classification
    // Note to self:
    // Here I tried to use juce::Thread::notify() to wake up the classification thread
    // This meant having a wait(-1) in a loop inside the classifier callback (instead of the successive wait)
    // Contrary to what was hinted in the JUCE forum, this causes mode switches in Xenomai so it is not rt safe. 
    // Old line: classificationThread.notify();
   #else
    classify(DemoProcessor::timbreClassifier,&(featureVector[0]), featureVector.size(),&(classificationOutputVector[0]), classificationOutputVector.size()); // Execute inference with the Interpreter of choice (see PluginProcessor.h)
    this->chrono_end = std::chrono::high_resolution_clock::now();
    this->classf_end = juce::Time::getMillisecondCounterHiRes();
    classificationFinished();
   #endif
}

void DemoProcessor::classificationFinished()
{
     /** LOG ADDITIONAL TIMESTAMP FOR LATENCY COMPUTATION **/
  #ifndef FAST_MODE_1
   #ifdef MEASURE_COMPUTATION_LATENCY
    #ifndef SEQUENTIAL_CLASSIFICATION
        this->chrono_end = std::chrono::high_resolution_clock::now();
        this->classf_end = juce::Time::getMillisecondCounterHiRes();
    #endif
    rtlogger.logValue("Classification started at ",this->classf_start);
    rtlogger.logValue("Classification stopped at ",this->classf_end);
    rtlogger.logValue("Chrono duration",std::to_string(std::chrono::duration_cast<std::chrono::microseconds>(chrono_end - chrono_start).count()).c_str());
    rtlogger.logValue("(Classification stopped ",(this->classf_end-this->latencyTime),"ms after onset detection");
   #endif
  #endif

    bool VERBOSERES = true,
         VERBOSE = true;

    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];
  
   #ifdef DEBUG_WITH_SPIKE
    rtlogger.logValue("CreatingLogSignal started at",juce::Time::getMillisecondCounterHiRes());
    squaredsinewt.playHalfwave();
    rtlogger.logValue("CreatingLogSignal stopped at",juce::Time::getMillisecondCounterHiRes());
   #endif

    // Simple argmax
    int prediction = 0;
    for(int i=0; i<classificationOutputVector.size(); ++i)
        if(classificationOutputVector[i] > classificationOutputVector[prediction])
            prediction = i;
    float confidence = classificationOutputVector[prediction];

   #ifndef FAST_MODE_1
    /** LOG CLASSIFICATION RESULTS AND TIME **/
    if (VERBOSERES)
    {
        snprintf(message,sizeof(message),"Result: Predicted class %d with confidence %f",prediction,confidence);
        rtlogger.logInfo(message);
    }

    /** LOG ADDITIONAL INFO (CONFIDENCE VALUES FOR ALL CLASSES) **/
    if (VERBOSE)
    {
        for (int j = 0; j < classificationOutputVector.size(); ++j)
        {
            snprintf(message,sizeof(message),"Class %d confidence %f",j,classificationOutputVector[j]);
            rtlogger.logInfo(message);
        }
    }
   #endif

    /** TRIGGER OUTPUT NOTE WHEN CONDITIONS ARE MET **/

    // double cMajorScale[8] = {440,493,523,587,659,698,783,880};
    // sinewt.playNote( cMajorScale[prediction] );
    
    // 0)   Kick (Palm on lower body)
    // 1)   Snare 1 (All fingers on lower side)
    // 2)   Tom (Thumb on higher body)
    // 3)   Snare 2 (Fingers on the muted strings, over the end of the fingerboard)
    // 4)   Natural Harmonics (Stop strings from playing the dominant frequency, letting harmonics ring)
    // 5)   Palm Mute (Muting partially the strings with the palm of the pick hand)
    // 6)   Pick Near Bridge (Playing toward the bridge/saddle)
    // 7)   Pick Over the Soundhole (Playing over the sound hole)

#ifndef DEBUG_WITH_SPIKE
    if(prediction == 0)
        sinewt.playNote( 1200 );
    else
        sinewt.playNote( 440 );
#endif

}

void DemoProcessor::computeFeatureVector()
{
    jassert(featureVector.size() == featParser->featureList.size());

    /*----------------------------------------------------------------------------------------------------------/
    |  1. Run all the feature extractors needed (not all features will be needed in the end, that is phase 2)   |
    /*---------------------------------------------------------------------------------------------------------*/
    for (const JsonConf::feature_extractor_t featureExtractorToRun : featParser->precomputationList) {
        switch (featureExtractorToRun)
        {
            case JsonConf::feature_extractor_t::ATTACK_TIME:
                /*------------------------------------/
                | Attack time                         |
                /------------------------------------*/
                attackTime.compute(&rPeakSampIdx,&rAttackStartIdx,&rAttackTime);
                break;
            case JsonConf::feature_extractor_t::BARK_SPEC_BRIGHTNESS:
                /*------------------------------------/
                | Bark Spectral Brightness            |
                /------------------------------------*/
                this->barkSpecBrightnessVal = barkSpecBrightness.compute();
                break;
            case JsonConf::feature_extractor_t::BARK_SPEC:
                /*------------------------------------/
                | Bark Spectrum                       |
                /------------------------------------*/
                barkSpecRes = barkSpec.compute();
                if (barkSpecRes.size() != BARKSPEC_RES_SIZE)
                    throw std::logic_error("Incorrect result vector size for barkSpec");
                break;
            case JsonConf::feature_extractor_t::BFCC:
                /*-------------------------------------/
                | Bark Frequency Cepstral Coefficients |
                /-------------------------------------*/
                bfccRes = bfcc.compute();
                if (bfccRes.size() != BFCC_RES_SIZE)
                    throw std::logic_error("Incorrect result vector size for bfcc");
                break;
            case JsonConf::feature_extractor_t::MFCC:
                mfcc.compute();
                mfccRes = this->mfcc.compute();
                if (mfccRes.size() != MFCC_RES_SIZE)
                    throw std::logic_error("Incorrect result vector size for mfcc");
                break;
            case JsonConf::feature_extractor_t::CEPSTRUM:
                /*-------------------------------------/
                | Cepstrum Coefficients                |
                /-------------------------------------*/
                cepstrumRes = this->cepstrum.compute();
                if (cepstrumRes.size() != CEPSTRUM_RES_SIZE)
                    throw std::logic_error("Incorrect result vector size for Cepstrum");
                break;
            case JsonConf::feature_extractor_t::PEAK_SAMPLE:
                /*------------------------------------/
                | Peak sample                         |
                /------------------------------------*/
                peakSample.compute(this->peak,this->peakIdx);
                break;
            case JsonConf::feature_extractor_t::ZERO_CROSSING:
                this->zeroCrossingVal = zeroCrossing.compute();
                break;
        }
    }

    // /*----------------------------------------------------------------------------------------------------------/
    // |  2. Fill the feature vector following the JSON configuration instructions                                 |
    // /*---------------------------------------------------------------------------------------------------------*/

    for (size_t idx = 0; idx < featParser->featureList.size(); ++idx) {
        const JsonConf::FeatureParser::Feature* item  = featParser->featureList[idx];

        const JsonConf::FeatureParser::NamedFeature* namedFeature = dynamic_cast<const JsonConf::FeatureParser::NamedFeature*>((JsonConf::FeatureParser::Feature*)item);
        const JsonConf::FeatureParser::VectorFeature* vectorFeature = dynamic_cast<const JsonConf::FeatureParser::VectorFeature*>((JsonConf::FeatureParser::Feature*)item);

        switch (item ->extractorType)
        {
            case JsonConf::feature_extractor_t::ATTACK_TIME:
                // std::cout << "Adding ATTACK_TIME feature" << std::endl;
                if (namedFeature == nullptr)
                    throw std::logic_error("Error extracting NamedFeature ATTACK_TIME");
                if (namedFeature->valuename == "peaksamp")
                    NULL; //this->featureVector[idx] = rPeakSampIdx;
                else if (namedFeature->valuename == "attackStartIdx")
                    NULL; //this->featureVector[idx] = rAttackStartIdx;
                else if (namedFeature->valuename == "value")
                    NULL; //this->featureVector[idx] = rAttackTime;
                else
                    throw std::logic_error("Error extracting NamedFeature ATTACK_TIME: feature name unknown ("+namedFeature->valuename+")");
                break;
            case JsonConf::feature_extractor_t::BARK_SPEC_BRIGHTNESS:
                // std::cout << "Adding BARK_SPEC_BRIGHTNESS feature" << std::endl;
                this->featureVector[idx] = this->barkSpecBrightnessVal;
                break;
            case JsonConf::feature_extractor_t::BARK_SPEC:
                // std::cout << "Adding BARK_SPEC feature" << std::endl;
                if (vectorFeature->index >= barkSpecRes.size())
                    throw std::logic_error("Error extracting BARK_SPEC: index requested is greater than the maximum value("+std::to_string(vectorFeature->index)+">"+std::to_string(barkSpecRes.size()-1)+")");
                this->featureVector[idx] = this->barkSpecRes[vectorFeature->index];
                break;
            case JsonConf::feature_extractor_t::BFCC:
                // std::cout << "Adding BFCC feature" << std::endl;
                if (vectorFeature->index >= bfccRes.size())
                    throw std::logic_error("Error extracting Feature BFCC: index requested is greater than the maximum value("+std::to_string(vectorFeature->index)+">"+std::to_string(bfccRes.size()-1)+")");
                this->featureVector[idx] = this->bfccRes[vectorFeature->index];
                break;
            case JsonConf::feature_extractor_t::MFCC:
                // std::cout << "Adding MFCC feature" << std::endl;
                if (vectorFeature->index >= mfccRes.size())
                    throw std::logic_error("Error extracting Feature MFCC: index requested is greater than the maximum value("+std::to_string(vectorFeature->index)+">"+std::to_string(mfccRes.size()-1)+")");
                this->featureVector[idx] = this->mfccRes[vectorFeature->index];
                break;
            case JsonConf::feature_extractor_t::CEPSTRUM:
                // std::cout << "Adding CEPSTRUM feature" << std::endl;
                if (vectorFeature->index >= cepstrumRes.size())
                    throw std::logic_error("Error extracting Feature CEPSTRUM: index requested is greater than the maximum value("+std::to_string(vectorFeature->index)+">"+std::to_string(cepstrumRes.size()-1)+")");
                this->featureVector[idx] = this->cepstrumRes[vectorFeature->index];
                break;
            case JsonConf::feature_extractor_t::PEAK_SAMPLE:
                if (namedFeature == nullptr)
                    throw std::logic_error("Error extracting PEAK_SAMPLE");
                if (namedFeature->valuename == "value")
                    this->featureVector[idx] = peak;
                else if (namedFeature->valuename == "index")
                    this->featureVector[idx] = peakIdx;
                else
                    throw std::logic_error("Error extracting NamedFeature PEAK_SAMPLE: feature name unknown ("+namedFeature->valuename+")");
                break;
            case JsonConf::feature_extractor_t::ZERO_CROSSING:
                // std::cout << "Adding ZERO_CROSSING feature" << std::endl;
                this->featureVector[idx] = this->zeroCrossingVal;
                break;
        }
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
