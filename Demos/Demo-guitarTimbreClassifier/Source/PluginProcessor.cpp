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

#include <vector>
#include <chrono>

#define MONO_CHANNEL 0
#define POST_ONSET_DELAY_MS 6.96 // This delay is infact then rounded to the closest multiple of the audio block period
                                 // In the case of 6.96ms at 48000Hz and 64 samples blocksizes, the closes delay is 
                                 // 6.66ms, corresponding to 5 audio block periods
#define DO_DELAY_ONSET // If not defined there is NO delay between onset detection and feature extraction

#ifdef PARALLEL_CLASSIFICATION
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
#ifdef PARALLEL_CLASSIFICATION
    ,
    classificationThread(&DemoProcessor::classificationData,&DemoProcessor::timbreClassifier)
#endif
{
  #ifndef FAST_MODE_1
   rtlogger.logInfo("Initializing Onset detector");
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
    bfccRes.resize(BFCC_RES_SIZE);
    cepstrumRes.resize(CEPSTRUM_RES_SIZE);

   #if defined(USE_TFLITE)
    rtlogger.logInfo("Neural Network interpreter: Tensorflow Lite");
   #elif defined(USE_RTNEURAL_CTIME)
    rtlogger.logInfo("Neural Network interpreter: Rt neural, Compile-time defined model");
   #elif defined(USE_RTNEURAL_RTIME)
    rtlogger.logInfo("Neural Network interpreter: Rt neural, Run-time interpreted model");
   #elif defined(USE_ONNX)
    rtlogger.logInfo("Neural Network interpreter: OnnxRuntime");
   #elif USE_TORCHSCRIPT
    rtlogger.logInfo("Neural Network interpreter: TorchScript");
   #else
    rtlogger.logInfo("ERROR, can't find a preprocessor directive with the neural interpreter to use");
   #endif
    rtlogger.logValue("Model Path: ",MODEL_PATH.c_str());

    /** SET UP CLASSIFIER **/
   #ifndef FAST_MODE_1
    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];
    snprintf(message,sizeof(message),"Model path set to '%s'",MODEL_PATH.c_str());
    rtlogger.logInfo(message);
   #endif
    DemoProcessor::timbreClassifier = createClassifier(MODEL_PATH);
   #ifndef FAST_MODE_1
    rtlogger.logInfo("Classifier object istantiated");
    /** START POLLING ROUTINE THAT WRITES TO FILE ALL THE LOG ENTRIES **/
    pollingTimer.startLogRoutine(100); // Call every 0.1 seconds
   #endif
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

    /*------------------------------------/
    | Other Feature extractors available  |
    /------------------------------------*/

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
    attackTime.store(buffer,(short int)MONO_CHANNEL);           // < /*-----------------------------------/
    barkSpecBrightness.store(buffer,(short int)MONO_CHANNEL);   // < |                                    |
    barkSpec.store(buffer,(short int)MONO_CHANNEL);             // < | Other Feature extractors available |
    mfcc.store(buffer,(short int)MONO_CHANNEL);                 // < |                                    |
    peakSample.store(buffer,(short int)MONO_CHANNEL);           // < |                                    |
    zeroCrossing.store(buffer,(short int)MONO_CHANNEL);         // < /-----------------------------------*/

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

   #ifdef PARALLEL_CLASSIFICATION
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
    this->computeFeatureVector();

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
    rtlogger.logValue("Classification started at ",juce::Time::getMillisecondCounterHiRes());
    rtlogger.logValue("(",(juce::Time::getMillisecondCounterHiRes()-latencyTime),"ms after onset detection");
   #endif
  #endif

   
   #ifdef PARALLEL_CLASSIFICATION
    DemoProcessor::classificationData.featureBuffer.write(featureVector);    // Write the feature vector to a ring buffer, for parallel classification
    // Note to self:
    // Here I tried to use juce::Thread::notify() to wake up the classification thread
    // This meant having a wait(-1) in a loop inside the classifier callback (instead of the successive wait)
    // Contrary to what was hinted in the JUCE forum, this causes mode switches in Xenomai so it is not rt safe. 
    // Old line: classificationThread.notify();
   #else
    classify(DemoProcessor::timbreClassifier,featureVector,classificationOutputVector); // Execute inference with the Interpreter of choice (see PluginProcessor.h)
    classificationFinished();
   #endif
}

void DemoProcessor::classificationFinished()
{
   #ifndef FAST_MODE_1
    rtlogger.logInfo("Classification Finished");
   #endif
   #ifdef DEBUG_WITH_SPIKE
    squaredsinewt.playHalfwave();
   #endif

    bool VERBOSERES = true,
         VERBOSE = true;

    char message[tid::RealTimeLogger::LogEntry::MESSAGE_LENGTH];

     /** LOG ADDITIONAL TIMESTAMP FOR LATENCY COMPUTATION **/
  #ifndef FAST_MODE_1
   #ifdef MEASURE_COMPUTATION_LATENCY
    rtlogger.logValue("Classification stopped at ",juce::Time::getMillisecondCounterHiRes());
    rtlogger.logValue("(Classification stopped ",(juce::Time::getMillisecondCounterHiRes()-latencyTime),"ms after onset detection");
   #endif
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

    // /* Manual selection before, automated script-generated code after*/

    // jassert(featureVector.size() == 180);

    // // Generated with computeFeatureCopier.py script
    // /*------------------------------------/
    // | Attack time                         |
    // /------------------------------------*/
    // attackTime.compute(&rPeakSampIdx,&rAttackStartIdx,&rAttackTime);
    // this->featureVector[0] = rAttackTime;
    // /*------------------------------------/
    // | Bark Spectral Brightness            |
    // /------------------------------------*/
    // this->featureVector[1] = barkSpecBrightness.compute();
    // /*------------------------------------/
    // | Bark Spectrum                       |
    // /------------------------------------*/
    // barkSpecRes = barkSpec.compute();
    // if (barkSpecRes.size() != BARKSPEC_RES_SIZE)
    //     throw std::logic_error("Incorrect result vector size for barkSpec");
    // this->featureVector[2] = barkSpecRes[1];
    // this->featureVector[3] = barkSpecRes[2];
    // this->featureVector[4] = barkSpecRes[3];
    // this->featureVector[5] = barkSpecRes[4];
    // this->featureVector[6] = barkSpecRes[5];
    // this->featureVector[7] = barkSpecRes[6];
    // this->featureVector[8] = barkSpecRes[7];
    // this->featureVector[9] = barkSpecRes[8];
    // this->featureVector[10] = barkSpecRes[9];
    // this->featureVector[11] = barkSpecRes[10];
    // this->featureVector[12] = barkSpecRes[11];
    // this->featureVector[13] = barkSpecRes[12];
    // this->featureVector[14] = barkSpecRes[13];
    // this->featureVector[15] = barkSpecRes[14];
    // this->featureVector[16] = barkSpecRes[15];
    // this->featureVector[17] = barkSpecRes[16];
    // this->featureVector[18] = barkSpecRes[17];
    // this->featureVector[19] = barkSpecRes[18];
    // this->featureVector[20] = barkSpecRes[19];
    // this->featureVector[21] = barkSpecRes[20];
    // this->featureVector[22] = barkSpecRes[21];
    // this->featureVector[23] = barkSpecRes[22];
    // this->featureVector[24] = barkSpecRes[23];
    // this->featureVector[25] = barkSpecRes[24];
    // this->featureVector[26] = barkSpecRes[25];
    // this->featureVector[27] = barkSpecRes[26];
    // this->featureVector[28] = barkSpecRes[27];
    // this->featureVector[29] = barkSpecRes[28];
    // this->featureVector[30] = barkSpecRes[29];
    // this->featureVector[31] = barkSpecRes[30];
    // this->featureVector[32] = barkSpecRes[31];
    // this->featureVector[33] = barkSpecRes[32];
    // this->featureVector[34] = barkSpecRes[33];
    // this->featureVector[35] = barkSpecRes[34];
    // this->featureVector[36] = barkSpecRes[35];
    // this->featureVector[37] = barkSpecRes[36];
    // this->featureVector[38] = barkSpecRes[37];
    // this->featureVector[39] = barkSpecRes[38];
    // this->featureVector[40] = barkSpecRes[39];
    // this->featureVector[41] = barkSpecRes[40];
    // this->featureVector[42] = barkSpecRes[41];
    // this->featureVector[43] = barkSpecRes[42];
    // this->featureVector[44] = barkSpecRes[43];
    // this->featureVector[45] = barkSpecRes[44];
    // this->featureVector[46] = barkSpecRes[45];
    // this->featureVector[47] = barkSpecRes[46];
    // this->featureVector[48] = barkSpecRes[47];
    // this->featureVector[49] = barkSpecRes[48];
    // this->featureVector[50] = barkSpecRes[49];
    // this->featureVector[51] = barkSpecRes[50];
    // /*-------------------------------------/
    // | Bark Frequency Cepstral Coefficients |
    // /-------------------------------------*/
    // bfccRes = bfcc.compute();
    // if (bfccRes.size() != BFCC_RES_SIZE)
    //     throw std::logic_error("Incorrect result vector size for bfcc");
    // this->featureVector[52] = bfccRes[2];
    // this->featureVector[52] = bfccRes[2];
    // this->featureVector[53] = bfccRes[3];
    // this->featureVector[53] = bfccRes[3];
    // this->featureVector[54] = bfccRes[4];
    // this->featureVector[54] = bfccRes[4];
    // this->featureVector[55] = bfccRes[5];
    // this->featureVector[55] = bfccRes[5];
    // this->featureVector[56] = bfccRes[6];
    // this->featureVector[56] = bfccRes[6];
    // this->featureVector[57] = bfccRes[7];
    // this->featureVector[57] = bfccRes[7];
    // this->featureVector[58] = bfccRes[8];
    // this->featureVector[58] = bfccRes[8];
    // this->featureVector[59] = bfccRes[9];
    // this->featureVector[59] = bfccRes[9];
    // this->featureVector[60] = bfccRes[10];
    // this->featureVector[60] = bfccRes[10];
    // this->featureVector[61] = bfccRes[11];
    // this->featureVector[61] = bfccRes[11];
    // this->featureVector[62] = bfccRes[12];
    // this->featureVector[62] = bfccRes[12];
    // this->featureVector[63] = bfccRes[13];
    // this->featureVector[63] = bfccRes[13];
    // this->featureVector[64] = bfccRes[15];
    // this->featureVector[64] = bfccRes[15];
    // this->featureVector[65] = bfccRes[16];
    // this->featureVector[65] = bfccRes[16];
    // this->featureVector[66] = bfccRes[17];
    // this->featureVector[66] = bfccRes[17];
    // this->featureVector[67] = bfccRes[18];
    // this->featureVector[67] = bfccRes[18];
    // this->featureVector[68] = bfccRes[19];
    // this->featureVector[68] = bfccRes[19];
    // this->featureVector[69] = bfccRes[20];
    // this->featureVector[69] = bfccRes[20];
    // this->featureVector[70] = bfccRes[21];
    // this->featureVector[70] = bfccRes[21];
    // this->featureVector[71] = bfccRes[25];
    // this->featureVector[71] = bfccRes[25];
    // this->featureVector[72] = bfccRes[26];
    // this->featureVector[72] = bfccRes[26];
    // this->featureVector[73] = bfccRes[27];
    // this->featureVector[73] = bfccRes[27];
    // this->featureVector[74] = bfccRes[28];
    // this->featureVector[74] = bfccRes[28];
    // this->featureVector[75] = bfccRes[29];
    // this->featureVector[75] = bfccRes[29];
    // this->featureVector[76] = bfccRes[30];
    // this->featureVector[76] = bfccRes[30];
    // this->featureVector[77] = bfccRes[31];
    // this->featureVector[77] = bfccRes[31];
    // this->featureVector[78] = bfccRes[35];
    // this->featureVector[78] = bfccRes[35];
    // this->featureVector[79] = bfccRes[36];
    // this->featureVector[79] = bfccRes[36];
    // this->featureVector[80] = bfccRes[37];
    // this->featureVector[80] = bfccRes[37];
    // this->featureVector[81] = bfccRes[39];
    // this->featureVector[81] = bfccRes[39];
    // this->featureVector[82] = bfccRes[40];
    // this->featureVector[82] = bfccRes[40];
    // this->featureVector[83] = bfccRes[42];
    // this->featureVector[83] = bfccRes[42];
    // this->featureVector[84] = bfccRes[43];
    // this->featureVector[84] = bfccRes[43];
    // this->featureVector[85] = bfccRes[44];
    // this->featureVector[85] = bfccRes[44];
    // this->featureVector[86] = bfccRes[45];
    // this->featureVector[86] = bfccRes[45];
    // this->featureVector[87] = bfccRes[46];
    // this->featureVector[87] = bfccRes[46];
    // this->featureVector[88] = bfccRes[48];
    // this->featureVector[88] = bfccRes[48];
    // /*-------------------------------------/
    // | Cepstrum Coefficients                |
    // /-------------------------------------*/
    // cepstrumRes = this->cepstrum.compute();
    // if (cepstrumRes.size() != CEPSTRUM_RES_SIZE)
    //     throw std::logic_error("Incorrect result vector size for Cepstrum");
    // this->featureVector[89] = cepstrumRes[1];
    // this->featureVector[90] = cepstrumRes[2];
    // this->featureVector[91] = cepstrumRes[3];
    // this->featureVector[92] = cepstrumRes[4];
    // this->featureVector[93] = cepstrumRes[5];
    // this->featureVector[94] = cepstrumRes[6];
    // this->featureVector[95] = cepstrumRes[7];
    // this->featureVector[96] = cepstrumRes[8];
    // this->featureVector[97] = cepstrumRes[9];
    // this->featureVector[98] = cepstrumRes[10];
    // this->featureVector[99] = cepstrumRes[11];
    // this->featureVector[100] = cepstrumRes[12];
    // this->featureVector[101] = cepstrumRes[13];
    // this->featureVector[102] = cepstrumRes[14];
    // this->featureVector[103] = cepstrumRes[15];
    // this->featureVector[104] = cepstrumRes[16];
    // this->featureVector[105] = cepstrumRes[17];
    // this->featureVector[106] = cepstrumRes[18];
    // this->featureVector[107] = cepstrumRes[19];
    // this->featureVector[108] = cepstrumRes[20];
    // this->featureVector[109] = cepstrumRes[21];
    // this->featureVector[110] = cepstrumRes[22];
    // this->featureVector[111] = cepstrumRes[23];
    // this->featureVector[112] = cepstrumRes[24];
    // this->featureVector[113] = cepstrumRes[25];
    // this->featureVector[114] = cepstrumRes[26];
    // this->featureVector[115] = cepstrumRes[27];
    // this->featureVector[116] = cepstrumRes[28];
    // this->featureVector[117] = cepstrumRes[29];
    // this->featureVector[118] = cepstrumRes[30];
    // this->featureVector[119] = cepstrumRes[31];
    // this->featureVector[120] = cepstrumRes[32];
    // this->featureVector[121] = cepstrumRes[33];
    // this->featureVector[122] = cepstrumRes[34];
    // this->featureVector[123] = cepstrumRes[35];
    // this->featureVector[124] = cepstrumRes[36];
    // this->featureVector[125] = cepstrumRes[37];
    // this->featureVector[126] = cepstrumRes[41];
    // this->featureVector[127] = cepstrumRes[42];
    // this->featureVector[128] = cepstrumRes[43];
    // this->featureVector[129] = cepstrumRes[44];
    // this->featureVector[130] = cepstrumRes[45];
    // this->featureVector[131] = cepstrumRes[46];
    // this->featureVector[132] = cepstrumRes[47];
    // this->featureVector[133] = cepstrumRes[48];
    // this->featureVector[134] = cepstrumRes[49];
    // this->featureVector[135] = cepstrumRes[54];
    // this->featureVector[136] = cepstrumRes[56];
    // this->featureVector[137] = cepstrumRes[59];
    // this->featureVector[138] = cepstrumRes[60];
    // this->featureVector[139] = cepstrumRes[67];
    // this->featureVector[140] = cepstrumRes[72];
    // this->featureVector[141] = cepstrumRes[86];
    // this->featureVector[142] = cepstrumRes[87];
    // this->featureVector[143] = cepstrumRes[108];
    // this->featureVector[144] = cepstrumRes[164];
    // this->featureVector[145] = cepstrumRes[205];
    // this->featureVector[146] = cepstrumRes[206];
    // /*------------------------------------/
    // | Mel Frequency Cepstral Coefficients |
    // /------------------------------------*/
    // mfcc.compute();
    // mfccRes = this->mfcc.compute();
    // if (mfccRes.size() != MFCC_RES_SIZE)
    //     throw std::logic_error("Incorrect result vector size for mfcc");
    // this->featureVector[147] = mfccRes[1];
    // this->featureVector[148] = mfccRes[2];
    // this->featureVector[149] = mfccRes[3];
    // this->featureVector[150] = mfccRes[4];
    // this->featureVector[151] = mfccRes[5];
    // this->featureVector[152] = mfccRes[6];
    // this->featureVector[153] = mfccRes[7];
    // this->featureVector[154] = mfccRes[8];
    // this->featureVector[155] = mfccRes[9];
    // this->featureVector[156] = mfccRes[10];
    // this->featureVector[157] = mfccRes[11];
    // this->featureVector[158] = mfccRes[12];
    // this->featureVector[159] = mfccRes[13];
    // this->featureVector[160] = mfccRes[14];
    // this->featureVector[161] = mfccRes[15];
    // this->featureVector[162] = mfccRes[16];
    // this->featureVector[163] = mfccRes[17];
    // this->featureVector[164] = mfccRes[18];
    // this->featureVector[165] = mfccRes[19];
    // this->featureVector[166] = mfccRes[20];
    // this->featureVector[167] = mfccRes[21];
    // this->featureVector[168] = mfccRes[22];
    // this->featureVector[169] = mfccRes[23];
    // this->featureVector[170] = mfccRes[24];
    // this->featureVector[171] = mfccRes[25];
    // this->featureVector[172] = mfccRes[26];
    // this->featureVector[173] = mfccRes[32];
    // this->featureVector[174] = mfccRes[33];
    // this->featureVector[175] = mfccRes[34];
    // this->featureVector[176] = mfccRes[35];
    // this->featureVector[177] = mfccRes[36];
    // /*------------------------------------/
    // | Peak sample                         |
    // /------------------------------------*/
    // peakSample.compute(peak,peakIdx);
    // this->featureVector[178] = peak;
    // /*------------------------------------/
    // | Zero Crossings                      |
    // /------------------------------------*/
    // this->featureVector[179] = zeroCrossing.compute();

    jassert(featureVector.size() == 180);

    // Generated with computeFeatureCopier.py script
    /*------------------------------------/
    | Attack time                         |
    /------------------------------------*/
    attackTime.compute(&rPeakSampIdx,&rAttackStartIdx,&rAttackTime);
    this->featureVector[0] = rAttackTime;
    /*------------------------------------/
    | Bark Spectral Brightness            |
    /------------------------------------*/
    this->featureVector[1] = barkSpecBrightness.compute();
    /*------------------------------------/
    | Bark Spectrum                       |
    /------------------------------------*/
    barkSpecRes = barkSpec.compute();
    if (barkSpecRes.size() != BARKSPEC_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for barkSpec");
    this->featureVector[2] = barkSpecRes[0]; //barkSpec_1
    this->featureVector[3] = barkSpecRes[1]; //barkSpec_2
    this->featureVector[4] = barkSpecRes[2]; //barkSpec_3
    this->featureVector[5] = barkSpecRes[3]; //barkSpec_4
    this->featureVector[6] = barkSpecRes[4]; //barkSpec_5
    this->featureVector[7] = barkSpecRes[5]; //barkSpec_6
    this->featureVector[8] = barkSpecRes[6]; //barkSpec_7
    this->featureVector[9] = barkSpecRes[7]; //barkSpec_8
    this->featureVector[10] = barkSpecRes[8]; //barkSpec_9
    this->featureVector[11] = barkSpecRes[9]; //barkSpec_10
    this->featureVector[12] = barkSpecRes[10]; //barkSpec_11
    this->featureVector[13] = barkSpecRes[11]; //barkSpec_12
    this->featureVector[14] = barkSpecRes[12]; //barkSpec_13
    this->featureVector[15] = barkSpecRes[13]; //barkSpec_14
    this->featureVector[16] = barkSpecRes[14]; //barkSpec_15
    this->featureVector[17] = barkSpecRes[15]; //barkSpec_16
    this->featureVector[18] = barkSpecRes[16]; //barkSpec_17
    this->featureVector[19] = barkSpecRes[17]; //barkSpec_18
    this->featureVector[20] = barkSpecRes[18]; //barkSpec_19
    this->featureVector[21] = barkSpecRes[19]; //barkSpec_20
    this->featureVector[22] = barkSpecRes[20]; //barkSpec_21
    this->featureVector[23] = barkSpecRes[21]; //barkSpec_22
    this->featureVector[24] = barkSpecRes[22]; //barkSpec_23
    this->featureVector[25] = barkSpecRes[23]; //barkSpec_24
    this->featureVector[26] = barkSpecRes[24]; //barkSpec_25
    this->featureVector[27] = barkSpecRes[25]; //barkSpec_26
    this->featureVector[28] = barkSpecRes[26]; //barkSpec_27
    this->featureVector[29] = barkSpecRes[27]; //barkSpec_28
    this->featureVector[30] = barkSpecRes[28]; //barkSpec_29
    this->featureVector[31] = barkSpecRes[29]; //barkSpec_30
    this->featureVector[32] = barkSpecRes[30]; //barkSpec_31
    this->featureVector[33] = barkSpecRes[31]; //barkSpec_32
    this->featureVector[34] = barkSpecRes[32]; //barkSpec_33
    this->featureVector[35] = barkSpecRes[33]; //barkSpec_34
    this->featureVector[36] = barkSpecRes[34]; //barkSpec_35
    this->featureVector[37] = barkSpecRes[35]; //barkSpec_36
    this->featureVector[38] = barkSpecRes[36]; //barkSpec_37
    this->featureVector[39] = barkSpecRes[37]; //barkSpec_38
    this->featureVector[40] = barkSpecRes[38]; //barkSpec_39
    this->featureVector[41] = barkSpecRes[39]; //barkSpec_40
    this->featureVector[42] = barkSpecRes[40]; //barkSpec_41
    this->featureVector[43] = barkSpecRes[41]; //barkSpec_42
    this->featureVector[44] = barkSpecRes[42]; //barkSpec_43
    this->featureVector[45] = barkSpecRes[43]; //barkSpec_44
    this->featureVector[46] = barkSpecRes[44]; //barkSpec_45
    this->featureVector[47] = barkSpecRes[45]; //barkSpec_46
    this->featureVector[48] = barkSpecRes[46]; //barkSpec_47
    this->featureVector[49] = barkSpecRes[47]; //barkSpec_48
    this->featureVector[50] = barkSpecRes[48]; //barkSpec_49
    this->featureVector[51] = barkSpecRes[49]; //barkSpec_50
    /*-------------------------------------/
    | Bark Frequency Cepstral Coefficients |
    /-------------------------------------*/
    bfccRes = bfcc.compute();
    if (bfccRes.size() != BFCC_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for bfcc");
    this->featureVector[52] = bfccRes[1]; //bfcc_2
    this->featureVector[53] = bfccRes[2]; //bfcc_3
    this->featureVector[54] = bfccRes[3]; //bfcc_4
    this->featureVector[55] = bfccRes[4]; //bfcc_5
    this->featureVector[56] = bfccRes[5]; //bfcc_6
    this->featureVector[57] = bfccRes[6]; //bfcc_7
    this->featureVector[58] = bfccRes[7]; //bfcc_8
    this->featureVector[59] = bfccRes[8]; //bfcc_9
    this->featureVector[60] = bfccRes[9]; //bfcc_10
    this->featureVector[61] = bfccRes[10]; //bfcc_11
    this->featureVector[62] = bfccRes[11]; //bfcc_12
    this->featureVector[63] = bfccRes[12]; //bfcc_13
    this->featureVector[64] = bfccRes[14]; //bfcc_15
    this->featureVector[65] = bfccRes[15]; //bfcc_16
    this->featureVector[66] = bfccRes[16]; //bfcc_17
    this->featureVector[67] = bfccRes[17]; //bfcc_18
    this->featureVector[68] = bfccRes[18]; //bfcc_19
    this->featureVector[69] = bfccRes[19]; //bfcc_20
    this->featureVector[70] = bfccRes[20]; //bfcc_21
    this->featureVector[71] = bfccRes[24]; //bfcc_25
    this->featureVector[72] = bfccRes[25]; //bfcc_26
    this->featureVector[73] = bfccRes[26]; //bfcc_27
    this->featureVector[74] = bfccRes[27]; //bfcc_28
    this->featureVector[75] = bfccRes[28]; //bfcc_29
    this->featureVector[76] = bfccRes[29]; //bfcc_30
    this->featureVector[77] = bfccRes[30]; //bfcc_31
    this->featureVector[78] = bfccRes[34]; //bfcc_35
    this->featureVector[79] = bfccRes[35]; //bfcc_36
    this->featureVector[80] = bfccRes[36]; //bfcc_37
    this->featureVector[81] = bfccRes[38]; //bfcc_39
    this->featureVector[82] = bfccRes[39]; //bfcc_40
    this->featureVector[83] = bfccRes[41]; //bfcc_42
    this->featureVector[84] = bfccRes[42]; //bfcc_43
    this->featureVector[85] = bfccRes[43]; //bfcc_44
    this->featureVector[86] = bfccRes[44]; //bfcc_45
    this->featureVector[87] = bfccRes[45]; //bfcc_46
    this->featureVector[88] = bfccRes[47]; //bfcc_48
    /*-------------------------------------/
    | Cepstrum Coefficients                |
    /-------------------------------------*/
    cepstrumRes = this->cepstrum.compute();
    if (cepstrumRes.size() != CEPSTRUM_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for Cepstrum");
    this->featureVector[89] = cepstrumRes[0]; //cepstrum_1
    this->featureVector[90] = cepstrumRes[1]; //cepstrum_2
    this->featureVector[91] = cepstrumRes[2]; //cepstrum_3
    this->featureVector[92] = cepstrumRes[3]; //cepstrum_4
    this->featureVector[93] = cepstrumRes[4]; //cepstrum_5
    this->featureVector[94] = cepstrumRes[5]; //cepstrum_6
    this->featureVector[95] = cepstrumRes[6]; //cepstrum_7
    this->featureVector[96] = cepstrumRes[7]; //cepstrum_8
    this->featureVector[97] = cepstrumRes[8]; //cepstrum_9
    this->featureVector[98] = cepstrumRes[9]; //cepstrum_10
    this->featureVector[99] = cepstrumRes[10]; //cepstrum_11
    this->featureVector[100] = cepstrumRes[11]; //cepstrum_12
    this->featureVector[101] = cepstrumRes[12]; //cepstrum_13
    this->featureVector[102] = cepstrumRes[13]; //cepstrum_14
    this->featureVector[103] = cepstrumRes[14]; //cepstrum_15
    this->featureVector[104] = cepstrumRes[15]; //cepstrum_16
    this->featureVector[105] = cepstrumRes[16]; //cepstrum_17
    this->featureVector[106] = cepstrumRes[17]; //cepstrum_18
    this->featureVector[107] = cepstrumRes[18]; //cepstrum_19
    this->featureVector[108] = cepstrumRes[19]; //cepstrum_20
    this->featureVector[109] = cepstrumRes[20]; //cepstrum_21
    this->featureVector[110] = cepstrumRes[21]; //cepstrum_22
    this->featureVector[111] = cepstrumRes[22]; //cepstrum_23
    this->featureVector[112] = cepstrumRes[23]; //cepstrum_24
    this->featureVector[113] = cepstrumRes[24]; //cepstrum_25
    this->featureVector[114] = cepstrumRes[25]; //cepstrum_26
    this->featureVector[115] = cepstrumRes[26]; //cepstrum_27
    this->featureVector[116] = cepstrumRes[27]; //cepstrum_28
    this->featureVector[117] = cepstrumRes[28]; //cepstrum_29
    this->featureVector[118] = cepstrumRes[29]; //cepstrum_30
    this->featureVector[119] = cepstrumRes[30]; //cepstrum_31
    this->featureVector[120] = cepstrumRes[31]; //cepstrum_32
    this->featureVector[121] = cepstrumRes[32]; //cepstrum_33
    this->featureVector[122] = cepstrumRes[33]; //cepstrum_34
    this->featureVector[123] = cepstrumRes[34]; //cepstrum_35
    this->featureVector[124] = cepstrumRes[35]; //cepstrum_36
    this->featureVector[125] = cepstrumRes[36]; //cepstrum_37
    this->featureVector[126] = cepstrumRes[40]; //cepstrum_41
    this->featureVector[127] = cepstrumRes[41]; //cepstrum_42
    this->featureVector[128] = cepstrumRes[42]; //cepstrum_43
    this->featureVector[129] = cepstrumRes[43]; //cepstrum_44
    this->featureVector[130] = cepstrumRes[44]; //cepstrum_45
    this->featureVector[131] = cepstrumRes[45]; //cepstrum_46
    this->featureVector[132] = cepstrumRes[46]; //cepstrum_47
    this->featureVector[133] = cepstrumRes[47]; //cepstrum_48
    this->featureVector[134] = cepstrumRes[48]; //cepstrum_49
    this->featureVector[135] = cepstrumRes[53]; //cepstrum_54
    this->featureVector[136] = cepstrumRes[55]; //cepstrum_56
    this->featureVector[137] = cepstrumRes[58]; //cepstrum_59
    this->featureVector[138] = cepstrumRes[59]; //cepstrum_60
    this->featureVector[139] = cepstrumRes[66]; //cepstrum_67
    this->featureVector[140] = cepstrumRes[71]; //cepstrum_72
    this->featureVector[141] = cepstrumRes[85]; //cepstrum_86
    this->featureVector[142] = cepstrumRes[86]; //cepstrum_87
    this->featureVector[143] = cepstrumRes[107]; //cepstrum_108
    this->featureVector[144] = cepstrumRes[163]; //cepstrum_164
    this->featureVector[145] = cepstrumRes[204]; //cepstrum_205
    this->featureVector[146] = cepstrumRes[205]; //cepstrum_206
    /*------------------------------------/
    | Mel Frequency Cepstral Coefficients |
    /------------------------------------*/
    mfcc.compute();
    mfccRes = this->mfcc.compute();
    if (mfccRes.size() != MFCC_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for mfcc"); //cepstrum_206
    this->featureVector[147] = mfccRes[0]; //mfcc_1
    this->featureVector[148] = mfccRes[1]; //mfcc_2
    this->featureVector[149] = mfccRes[2]; //mfcc_3
    this->featureVector[150] = mfccRes[3]; //mfcc_4
    this->featureVector[151] = mfccRes[4]; //mfcc_5
    this->featureVector[152] = mfccRes[5]; //mfcc_6
    this->featureVector[153] = mfccRes[6]; //mfcc_7
    this->featureVector[154] = mfccRes[7]; //mfcc_8
    this->featureVector[155] = mfccRes[8]; //mfcc_9
    this->featureVector[156] = mfccRes[9]; //mfcc_10
    this->featureVector[157] = mfccRes[10]; //mfcc_11
    this->featureVector[158] = mfccRes[11]; //mfcc_12
    this->featureVector[159] = mfccRes[12]; //mfcc_13
    this->featureVector[160] = mfccRes[13]; //mfcc_14
    this->featureVector[161] = mfccRes[14]; //mfcc_15
    this->featureVector[162] = mfccRes[15]; //mfcc_16
    this->featureVector[163] = mfccRes[16]; //mfcc_17
    this->featureVector[164] = mfccRes[17]; //mfcc_18
    this->featureVector[165] = mfccRes[18]; //mfcc_19
    this->featureVector[166] = mfccRes[19]; //mfcc_20
    this->featureVector[167] = mfccRes[20]; //mfcc_21
    this->featureVector[168] = mfccRes[21]; //mfcc_22
    this->featureVector[169] = mfccRes[22]; //mfcc_23
    this->featureVector[170] = mfccRes[23]; //mfcc_24
    this->featureVector[171] = mfccRes[24]; //mfcc_25
    this->featureVector[172] = mfccRes[25]; //mfcc_26
    this->featureVector[173] = mfccRes[31]; //mfcc_32
    this->featureVector[174] = mfccRes[32]; //mfcc_33
    this->featureVector[175] = mfccRes[33]; //mfcc_34
    this->featureVector[176] = mfccRes[34]; //mfcc_35
    this->featureVector[177] = mfccRes[35]; //mfcc_36
    /*------------------------------------/
    | Peak sample                         |
    /------------------------------------*/
    peakSample.compute(peak,peakIdx);
    this->featureVector[178] = peak;
    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    this->featureVector[179] = zeroCrossing.compute();


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
