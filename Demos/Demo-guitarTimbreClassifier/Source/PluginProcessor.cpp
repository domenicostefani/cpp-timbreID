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
        rtlogger.logInfo("An exception has occurred while buffering: ",e.what());
    } catch(...) {
        rtlogger.logInfo("An unknwn exception has occurred while buffering: ");
    }
    /** CLEAR THE BUFFER (OPTIONAL) **/
    buffer.clear(); // Uncomment to let input pass through

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
        snprintf(message,sizeof(message),"block nr %lld processed",logProcessblock_fixedCounter);
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
    | 2. CLASSIFY TIMBRE | (Aka running inference)
    /-------------------*/
    /** START ADDITIONAL "CHRONOMETER" JUST FOR CLASSIFICATION **/
    auto start = std::chrono::high_resolution_clock::now();
    /** INVOKE CLASSIFIER**/
    int prediction = classify(timbreClassifier,featureVector,classificationOutputVector);
    /** STOP CLASSIFICATION "CHRONOMETER" **/
    auto stop = std::chrono::high_resolution_clock::now();

    /** LOG ADDITIONAL TIMESTAMP FOR LATENCY COMPUTATION
        TODO: Improve logg clarity, precision and remove duplicates (like chrono library) **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    snprintf(message,sizeof(message),"Prediction stopped %f ms after onset detection",(juce::Time::getMillisecondCounterHiRes() - latencyTime));
    rtlogger.logInfo(message);
   #endif

    float confidence = classificationOutputVector[prediction];

    /** LOG CLASSIFICATION RESULTS AND TIME **/
    if (VERBOSERES)
    {
        snprintf(message,sizeof(message),"Result: Predicted class %d with confidence %f",prediction,confidence);
        rtlogger.logInfo(message);
        snprintf(message,sizeof(message),"(std::chrono) Classification took %ld us or %ld ms", \
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

    double cMajorScale[8] = {440,493,523,587,659,698,783,880};
    sinewt.playNote( cMajorScale[prediction] );
}

void DemoProcessor::computeFeatureVector()
{
    // const unsigned int BARKSPEC_TO_KEEP = 50;
    // const unsigned int BFCC_TO_KEEP = 40;
    // const unsigned int CEPSTRUM_TO_KEEP = 60;
    // const unsigned int MFCC_TO_KEEP = 30;
    // int lastfreeindex = 0;

    // /*------------------------------------/
    // | Attack time                         |
    // /------------------------------------*/
    // attackTime.compute(&rPeakSampIdx,&rAttackStartIdx,&rAttackTime);
    // this->featureVector[lastfreeindex++] = rAttackTime;
    
    // /*------------------------------------/
    // | Bark Spectral Brightness            |
    // /------------------------------------*/
    // this->featureVector[lastfreeindex++] = barkSpecBrightness.compute();
    // /*------------------------------------/
    // | Bark Spectrum                       |
    // /------------------------------------*/
    // barkSpecRes = barkSpec.compute();
    // if (barkSpecRes.size() != BARKSPEC_RES_SIZE)
    //     throw std::logic_error("Incorrect result vector size for barkSpec");
    // /* Copy only part of the BarkSpec coefficients */
    // for(int i=0; i<BARKSPEC_TO_KEEP; ++i)
    //     this->featureVector[lastfreeindex+i] = barkSpecRes[i];
    // lastfreeindex += BARKSPEC_TO_KEEP;
    // /*-------------------------------------/
    // | Bark Frequency Cepstral Coefficients |
    // /-------------------------------------*/
    // bfccRes = this->bfcc.compute();
    // if (bfccRes.size() != BFCC_RES_SIZE)
    //     throw std::logic_error("Incorrect result vector size for bfcc");
    // /* Copy only part of the bfcc coefficients */
    // for(int i=0; i<BFCC_TO_KEEP; ++i)
    //     this->featureVector[lastfreeindex+i] = bfccRes[i];
    // lastfreeindex += BFCC_TO_KEEP;
    // /*-------------------------------------/
    // | Cepstrum Coefficients                |
    // /-------------------------------------*/
    // cepstrumRes = this->cepstrum.compute();
    // if (cepstrumRes.size() != CEPSTRUM_RES_SIZE)
    //     throw std::logic_error("Incorrect result vector size for Cepstrum");
    // /* Copy only part of the Cespstrum coefficients */
    // for(int i=0; i<CEPSTRUM_TO_KEEP; ++i)
    //     this->featureVector[lastfreeindex+i] = cepstrumRes[i];
    // lastfreeindex += CEPSTRUM_TO_KEEP;
    // /*------------------------------------/
    // | Mel Frequency Cepstral Coefficients |
    // /------------------------------------*/
    // mfcc.compute();
    // mfccRes = this->mfcc.compute();
    // if (mfccRes.size() != MFCC_RES_SIZE)
    //     throw std::logic_error("Incorrect result vector size for mfcc");
    // /* Copy only part of the Mfcc coefficients */
    // for(int i=0; i<MFCC_TO_KEEP; ++i)
    //     this->featureVector[lastfreeindex+i] = mfccRes[i];
    // lastfreeindex += MFCC_TO_KEEP;
    // /*------------------------------------/
    // | Peak sample                         |
    // /------------------------------------*/
    // peakSample.compute(peak,peakIdx);
    // this->featureVector[lastfreeindex++] = peak;

    // /*------------------------------------/
    // | Zero Crossings                      |
    // /------------------------------------*/
    // // zeroCrossing.compute();

    // jassert(lastfreeindex == featureVector.size());
    // jassert(lastfreeindex == VECTOR_SIZE);

    /* Manual selection before, automated script-generated code after*/

    jassert(featureVector.size() == 190);

    if (featureVector.size() != 190)
        throw std::logic_error("Feature vector has size"+std::to_string(featureVector.size())+" instead of 190");

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
    this->featureVector[2] = barkSpecRes[1];
    this->featureVector[3] = barkSpecRes[2];
    this->featureVector[4] = barkSpecRes[3];
    this->featureVector[5] = barkSpecRes[4];
    this->featureVector[6] = barkSpecRes[5];
    this->featureVector[7] = barkSpecRes[6];
    this->featureVector[8] = barkSpecRes[7];
    this->featureVector[9] = barkSpecRes[8];
    this->featureVector[10] = barkSpecRes[9];
    this->featureVector[11] = barkSpecRes[10];
    this->featureVector[12] = barkSpecRes[11];
    this->featureVector[13] = barkSpecRes[12];
    this->featureVector[14] = barkSpecRes[13];
    this->featureVector[15] = barkSpecRes[14];
    this->featureVector[16] = barkSpecRes[15];
    this->featureVector[17] = barkSpecRes[16];
    this->featureVector[18] = barkSpecRes[17];
    this->featureVector[19] = barkSpecRes[18];
    this->featureVector[20] = barkSpecRes[19];
    this->featureVector[21] = barkSpecRes[20];
    this->featureVector[22] = barkSpecRes[21];
    this->featureVector[23] = barkSpecRes[22];
    this->featureVector[24] = barkSpecRes[23];
    this->featureVector[25] = barkSpecRes[24];
    this->featureVector[26] = barkSpecRes[25];
    this->featureVector[27] = barkSpecRes[26];
    this->featureVector[28] = barkSpecRes[27];
    this->featureVector[29] = barkSpecRes[28];
    this->featureVector[30] = barkSpecRes[29];
    this->featureVector[31] = barkSpecRes[30];
    this->featureVector[32] = barkSpecRes[31];
    this->featureVector[33] = barkSpecRes[32];
    this->featureVector[34] = barkSpecRes[33];
    this->featureVector[35] = barkSpecRes[34];
    this->featureVector[36] = barkSpecRes[35];
    this->featureVector[37] = barkSpecRes[36];
    this->featureVector[38] = barkSpecRes[37];
    this->featureVector[39] = barkSpecRes[38];
    this->featureVector[40] = barkSpecRes[39];
    this->featureVector[41] = barkSpecRes[40];
    this->featureVector[42] = barkSpecRes[41];
    this->featureVector[43] = barkSpecRes[42];
    this->featureVector[44] = barkSpecRes[43];
    this->featureVector[45] = barkSpecRes[44];
    this->featureVector[46] = barkSpecRes[45];
    this->featureVector[47] = barkSpecRes[46];
    this->featureVector[48] = barkSpecRes[47];
    this->featureVector[49] = barkSpecRes[48];
    this->featureVector[50] = barkSpecRes[49];
    this->featureVector[51] = barkSpecRes[50];
    /*-------------------------------------/
    | Bark Frequency Cepstral Coefficients |
    /-------------------------------------*/
    bfccRes = bfcc.compute();
    if (bfccRes.size() != BFCC_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for bfcc");
    this->featureVector[52] = bfccRes[2];
    this->featureVector[52] = bfccRes[2];
    this->featureVector[53] = bfccRes[3];
    this->featureVector[53] = bfccRes[3];
    this->featureVector[54] = bfccRes[4];
    this->featureVector[54] = bfccRes[4];
    this->featureVector[55] = bfccRes[5];
    this->featureVector[55] = bfccRes[5];
    this->featureVector[56] = bfccRes[6];
    this->featureVector[56] = bfccRes[6];
    this->featureVector[57] = bfccRes[7];
    this->featureVector[57] = bfccRes[7];
    this->featureVector[58] = bfccRes[8];
    this->featureVector[58] = bfccRes[8];
    this->featureVector[59] = bfccRes[9];
    this->featureVector[59] = bfccRes[9];
    this->featureVector[60] = bfccRes[10];
    this->featureVector[60] = bfccRes[10];
    this->featureVector[61] = bfccRes[11];
    this->featureVector[61] = bfccRes[11];
    this->featureVector[62] = bfccRes[12];
    this->featureVector[62] = bfccRes[12];
    this->featureVector[63] = bfccRes[13];
    this->featureVector[63] = bfccRes[13];
    this->featureVector[64] = bfccRes[15];
    this->featureVector[64] = bfccRes[15];
    this->featureVector[65] = bfccRes[16];
    this->featureVector[65] = bfccRes[16];
    this->featureVector[66] = bfccRes[17];
    this->featureVector[66] = bfccRes[17];
    this->featureVector[67] = bfccRes[18];
    this->featureVector[67] = bfccRes[18];
    this->featureVector[68] = bfccRes[19];
    this->featureVector[68] = bfccRes[19];
    this->featureVector[69] = bfccRes[20];
    this->featureVector[69] = bfccRes[20];
    this->featureVector[70] = bfccRes[21];
    this->featureVector[70] = bfccRes[21];
    this->featureVector[71] = bfccRes[25];
    this->featureVector[71] = bfccRes[25];
    this->featureVector[72] = bfccRes[26];
    this->featureVector[72] = bfccRes[26];
    this->featureVector[73] = bfccRes[27];
    this->featureVector[73] = bfccRes[27];
    this->featureVector[74] = bfccRes[28];
    this->featureVector[74] = bfccRes[28];
    this->featureVector[75] = bfccRes[29];
    this->featureVector[75] = bfccRes[29];
    this->featureVector[76] = bfccRes[30];
    this->featureVector[76] = bfccRes[30];
    this->featureVector[77] = bfccRes[31];
    this->featureVector[77] = bfccRes[31];
    this->featureVector[78] = bfccRes[34];
    this->featureVector[78] = bfccRes[34];
    this->featureVector[79] = bfccRes[35];
    this->featureVector[79] = bfccRes[35];
    this->featureVector[80] = bfccRes[36];
    this->featureVector[80] = bfccRes[36];
    this->featureVector[81] = bfccRes[37];
    this->featureVector[81] = bfccRes[37];
    this->featureVector[82] = bfccRes[38];
    this->featureVector[82] = bfccRes[38];
    this->featureVector[83] = bfccRes[39];
    this->featureVector[83] = bfccRes[39];
    this->featureVector[84] = bfccRes[40];
    this->featureVector[84] = bfccRes[40];
    this->featureVector[85] = bfccRes[42];
    this->featureVector[85] = bfccRes[42];
    this->featureVector[86] = bfccRes[43];
    this->featureVector[86] = bfccRes[43];
    this->featureVector[87] = bfccRes[44];
    this->featureVector[87] = bfccRes[44];
    this->featureVector[88] = bfccRes[45];
    this->featureVector[88] = bfccRes[45];
    this->featureVector[89] = bfccRes[46];
    this->featureVector[89] = bfccRes[46];
    this->featureVector[90] = bfccRes[47];
    this->featureVector[90] = bfccRes[47];
    this->featureVector[91] = bfccRes[48];
    this->featureVector[91] = bfccRes[48];
    /*-------------------------------------/
    | Cepstrum Coefficients                |
    /-------------------------------------*/
    cepstrumRes = this->cepstrum.compute();
    if (cepstrumRes.size() != CEPSTRUM_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for Cepstrum");
    this->featureVector[92] = cepstrumRes[1];
    this->featureVector[93] = cepstrumRes[2];
    this->featureVector[94] = cepstrumRes[3];
    this->featureVector[95] = cepstrumRes[4];
    this->featureVector[96] = cepstrumRes[5];
    this->featureVector[97] = cepstrumRes[6];
    this->featureVector[98] = cepstrumRes[7];
    this->featureVector[99] = cepstrumRes[8];
    this->featureVector[100] = cepstrumRes[9];
    this->featureVector[101] = cepstrumRes[10];
    this->featureVector[102] = cepstrumRes[11];
    this->featureVector[103] = cepstrumRes[12];
    this->featureVector[104] = cepstrumRes[13];
    this->featureVector[105] = cepstrumRes[14];
    this->featureVector[106] = cepstrumRes[15];
    this->featureVector[107] = cepstrumRes[16];
    this->featureVector[108] = cepstrumRes[17];
    this->featureVector[109] = cepstrumRes[18];
    this->featureVector[110] = cepstrumRes[19];
    this->featureVector[111] = cepstrumRes[20];
    this->featureVector[112] = cepstrumRes[21];
    this->featureVector[113] = cepstrumRes[22];
    this->featureVector[114] = cepstrumRes[23];
    this->featureVector[115] = cepstrumRes[24];
    this->featureVector[116] = cepstrumRes[25];
    this->featureVector[117] = cepstrumRes[26];
    this->featureVector[118] = cepstrumRes[27];
    this->featureVector[119] = cepstrumRes[28];
    this->featureVector[120] = cepstrumRes[29];
    this->featureVector[121] = cepstrumRes[30];
    this->featureVector[122] = cepstrumRes[31];
    this->featureVector[123] = cepstrumRes[32];
    this->featureVector[124] = cepstrumRes[33];
    this->featureVector[125] = cepstrumRes[34];
    this->featureVector[126] = cepstrumRes[35];
    this->featureVector[127] = cepstrumRes[36];
    this->featureVector[128] = cepstrumRes[37];
    this->featureVector[129] = cepstrumRes[38];
    this->featureVector[130] = cepstrumRes[39];
    this->featureVector[131] = cepstrumRes[41];
    this->featureVector[132] = cepstrumRes[42];
    this->featureVector[133] = cepstrumRes[43];
    this->featureVector[134] = cepstrumRes[44];
    this->featureVector[135] = cepstrumRes[45];
    this->featureVector[136] = cepstrumRes[46];
    this->featureVector[137] = cepstrumRes[47];
    this->featureVector[138] = cepstrumRes[48];
    this->featureVector[139] = cepstrumRes[49];
    this->featureVector[140] = cepstrumRes[51];
    this->featureVector[141] = cepstrumRes[53];
    this->featureVector[142] = cepstrumRes[54];
    this->featureVector[143] = cepstrumRes[56];
    this->featureVector[144] = cepstrumRes[59];
    this->featureVector[145] = cepstrumRes[60];
    this->featureVector[146] = cepstrumRes[67];
    this->featureVector[147] = cepstrumRes[70];
    this->featureVector[148] = cepstrumRes[72];
    this->featureVector[149] = cepstrumRes[86];
    this->featureVector[150] = cepstrumRes[87];
    this->featureVector[151] = cepstrumRes[108];
    this->featureVector[152] = cepstrumRes[129];
    this->featureVector[153] = cepstrumRes[164];
    this->featureVector[154] = cepstrumRes[205];
    this->featureVector[155] = cepstrumRes[206];
    /*------------------------------------/
    | Mel Frequency Cepstral Coefficients |
    /------------------------------------*/
    mfcc.compute();
    mfccRes = this->mfcc.compute();
    if (mfccRes.size() != MFCC_RES_SIZE)
        throw std::logic_error("Incorrect result vector size for mfcc");
    this->featureVector[156] = mfccRes[1];
    this->featureVector[157] = mfccRes[2];
    this->featureVector[158] = mfccRes[3];
    this->featureVector[159] = mfccRes[4];
    this->featureVector[160] = mfccRes[5];
    this->featureVector[161] = mfccRes[6];
    this->featureVector[162] = mfccRes[7];
    this->featureVector[163] = mfccRes[8];
    this->featureVector[164] = mfccRes[9];
    this->featureVector[165] = mfccRes[10];
    this->featureVector[166] = mfccRes[11];
    this->featureVector[167] = mfccRes[12];
    this->featureVector[168] = mfccRes[13];
    this->featureVector[169] = mfccRes[14];
    this->featureVector[170] = mfccRes[15];
    this->featureVector[171] = mfccRes[16];
    this->featureVector[172] = mfccRes[17];
    this->featureVector[173] = mfccRes[18];
    this->featureVector[174] = mfccRes[19];
    this->featureVector[175] = mfccRes[20];
    this->featureVector[176] = mfccRes[21];
    this->featureVector[177] = mfccRes[22];
    this->featureVector[178] = mfccRes[23];
    this->featureVector[179] = mfccRes[24];
    this->featureVector[180] = mfccRes[25];
    this->featureVector[181] = mfccRes[26];
    this->featureVector[182] = mfccRes[28];
    this->featureVector[183] = mfccRes[32];
    this->featureVector[184] = mfccRes[33];
    this->featureVector[185] = mfccRes[34];
    this->featureVector[186] = mfccRes[35];
    this->featureVector[187] = mfccRes[36];
    /*------------------------------------/
    | Peak sample                         |
    /------------------------------------*/
    peakSample.compute(peak,peakIdx);
    this->featureVector[188] = peak;
    /*------------------------------------/
    | Zero Crossings                      |
    /------------------------------------*/
    this->featureVector[189] = zeroCrossing.compute();

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
