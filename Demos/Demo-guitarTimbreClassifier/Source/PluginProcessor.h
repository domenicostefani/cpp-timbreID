/*
  ==============================================================================
     Plugin Processor

     DEMO PROJECT - TimbreID - Guitar Timbre Classifier

     Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
     Date: 24th August 2020
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"
#include "wavetableSine.h"

#include "liteclassifier.h"
#include "postOnsetTimer.h"

#define USE_AUBIO_ONSET //If this commented, the bark onset detector is used, otherwise the aubio onset module is used
#define MEASURE_COMPUTATION_LATENCY
// #define DEBUG_WHICH_CHANNEL

//==============================================================================
/**
*/
class DemoProcessor : public AudioProcessor,
#ifdef USE_AUBIO_ONSET
                      public tid::aubio::Onset<float>::Listener
#else
                      public tid::Bark<float>::Listener
#endif
{
public:
    //===================== ONSET DETECTION PARAMS =============================

   #ifdef USE_AUBIO_ONSET
    //
    // More about the parameter choice at
    // https://github.com/domenicostefani/aubioonset-performanceanalysis/blob/main/report/complessive/Aubio_Study.pdf
    //
    const unsigned int AUBIO_WINDOW_SIZE = 256;
    const unsigned int AUBIO_HOP_SIZE = 64;
    const float ONSET_THRESHOLD = 1.21f;
    const float ONSET_MINIOI = 0.02f;  //20 ms debounce
    const float SILENCE_THRESHOLD = -53.0f;
    const tid::aubio::OnsetMethod ONSET_METHOD = tid::aubio::OnsetMethod::mkl;
    const bool DISABLE_ADAPTIVE_WHITENING = true; // remember to implement in initialization module
   #else
    const unsigned int BARK_WINDOW_SIZE = 1024;
    const unsigned int BARK_HOP_SIZE = 128;
   #endif

    //==================== FEATURE EXTRACTION PARAMS ===========================
    const unsigned int FEATUREEXT_WINDOW_SIZE = 704; // 11 Blocks of 64samples, 14.66ms
    const float BARK_SPACING = 0.5;
    const float BARK_BOUNDARY = 8.5;
    const float MEL_SPACING = 100;


    //========================= ONSET DETECTION ================================
   #ifdef USE_AUBIO_ONSET
    tid::aubio::Onset<float> aubioOnset{AUBIO_WINDOW_SIZE,
                                        AUBIO_HOP_SIZE,
                                        ONSET_THRESHOLD,
                                        ONSET_MINIOI,
                                        SILENCE_THRESHOLD,
                                        ONSET_METHOD};
    void onsetDetected (tid::aubio::Onset<float> *);
   #else
    /**    Initialize the onset detector      **/
    tid::Bark<float> bark{BARK_WINDOW_SIZE,
                          BARK_HOP_SIZE,
                          BARK_SPACING};
    void onsetDetected (tid::Bark<float>* bark);
   #endif

    PostOnsetTimer postOnsetTimer;
    void onsetDetectedRoutine();

    //======================== FEATURE EXTRACTION ==============================
    tid::Bfcc<float> bfcc{FEATUREEXT_WINDOW_SIZE, BARK_SPACING};
    tid::Cepstrum<float> cepstrum{FEATUREEXT_WINDOW_SIZE};
    tid::AttackTime<float> attackTime;
    tid::BarkSpecBrightness<float> barkSpecBrightness{FEATUREEXT_WINDOW_SIZE,
                                                      BARK_SPACING,
                                                      BARK_BOUNDARY};
    tid::BarkSpec<float> barkSpec{FEATUREEXT_WINDOW_SIZE, BARK_SPACING};
    tid::Mfcc<float> mfcc{FEATUREEXT_WINDOW_SIZE, MEL_SPACING};
    tid::PeakSample<float> peakSample{FEATUREEXT_WINDOW_SIZE};
    tid::ZeroCrossing<float> zeroCrossing{FEATUREEXT_WINDOW_SIZE};

    /**    Feature Vector    **/
    const unsigned int VECTOR_SIZE = 190; // Number of features (preallocation)
    std::vector<float> featureVector;     // Vector that is preallocated
    // attack time
    unsigned long int rPeakSampIdx, rAttackStartIdx; 
    float rAttackTime;
    // barkSpec
    const int BARKSPEC_RES_SIZE = 50;
    std::vector<float> barkSpecRes;
    // bfcc
    const int BFCC_RES_SIZE = 50;
    std::vector<float> bfccRes;
    // cepstum
    const int CEPSTRUM_RES_SIZE = 353; // WindowSize/2+1 (It would be 513 with 1025
    std::vector<float> cepstrumRes;
    // mfcc
    const int MFCC_RES_SIZE = 38;
    std::vector<float> mfccRes;
    // peak sample    
    float peak;
    unsigned long int peakIdx;
    // Compose vector
    void computeFeatureVector();

    //========================== CLASSIFICATION ================================
    ClassifierPtr timbreClassifier; // Tensorflow interpreter
    const std::string TFLITE_MODEL_PATH =  "/udata/tensorflow/model.tflite";
    const int N_OUTPUT_CLASSES = 8;
    std::vector<float> classificationOutputVector;

    //============================== OTHER =====================================
    /**    Debugging    */
   #ifdef DEBUG_WHICH_CHANNEL
    uint32 logCounter = 0;  //To debug which channel to use
   #endif
    /**    Profiling    **/
   #ifdef MEASURE_COMPUTATION_LATENCY
    double latencyTime = 0;
    uint64 logProcessblock_counter = 0, logProcessblock_fixedCounter = 0;
    const uint64 logProcessblock_period = 48000/64 * 5; // Log every 5 second the duration of the processblock routine (SampleRate/BlockSize)
   #endif

    /**    Logging    **/
    /*
     * Since many log messages need to be written from the Audio thread, there
     * is a real-time safe logger that uses a FIFO queue as a ring buffer.

     * Messages are written to the queue with:
     * rtlogger.logInfo(message); // message is a char array of size RealTimeLogger::LogEntry::MESSAGE_LENGTH+1
     * or
     * rtlogger.logInfo(message, timeAtStart, timeAtEnd); // both times can be obtained with Time::getHighResolutionTicks();
     *
     * Messages have to be consumed with:
     * rtlogger.pop(le)
     *
     * Log Entries are consumed and written to file with a Juce FileLogger from a
     * low priority thread (timer callback), hence why a FileLogger is used.
    */
    tid::RealTimeLogger rtlogger{"main-app"}; // Real time logger that queues messages

   #ifdef USE_AUBIO_ONSET
    std::vector<tid::RealTimeLogger*> loggerList = {&rtlogger, aubioOnset.getLoggerPtr()};
   #else
    std::vector<tid::RealTimeLogger*> loggerList = {&rtlogger, bark.getLoggerPtr()};
   #endif
    std::unique_ptr<FileLogger> fileLogger; // Logger that writes RT entries to file SAFELY (Outside rt thread
    std::string LOG_PATH = "/tmp/";
    std::string LOG_FILENAME = "guitarTimbreClassifier";
    const int64 highResFrequency = Time::getHighResolutionTicksPerSecond();

    /**
     * Timer that calls a function every @interval milliseconds.
     * It is used to consume log entries and write them to file safely
    */
    typedef std::function<void (void)> TimerEventCallback;
    class PollingTimer : public Timer
    {
    public:
        PollingTimer(TimerEventCallback cb) { this->cb = cb; }
        void startLogRoutine() { Timer::startTimer(this->interval); }
        void startLogRoutine(int interval) { this->interval = interval; Timer::startTimer(this->interval); }
        void timerCallback() override { cb(); }
    private:
        TimerEventCallback cb;
        int interval = 500; // Half second default polling interval
    };

    /** POLLING ROUTINE (called at regular intervals by the timer) **/
    void logPollingRoutine()
    {
        /** INITIALIZE THE FILELOGGER IF NECESSARY **/
        if (!this->fileLogger)
            this->fileLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(LOG_PATH, LOG_FILENAME, tIDLib::LOG_EXTENSION, "Guitar Timbre Classifier"));

        tid::RealTimeLogger::LogEntry le;
        for(tid::RealTimeLogger* loggerpointer : this->loggerList) // Cycle through all loggers subscribed to the list
        {
            std::string moduleName = loggerpointer->getName();
            while (loggerpointer->pop(le))  // Continue if there are log entries in the rtlogger queue
            {
                /** CONSUME RTLOGGER ENTRIES AND WRITE THEM TO FILE **/
                std::string msg = moduleName + "\t|\t";
                msg += le.message;
                double start,end,diff;
                start = (le.timeAtStart * 1.0) / this->highResFrequency;
                end = (le.timeAtEnd * 1.0) / this->highResFrequency;
                diff = end-start;
                if (start != 0)
                {
                    msg += " | start: " + std::to_string(start);
                    msg += " | end: " + std::to_string(end);
                    msg += " | difference: " + std::to_string(diff) + "s";
                }
                fileLogger->logMessage(msg);
            }
        }
    }

    /** Instantiate polling timer and pass callback **/
    PollingTimer pollingTimer{[this]{logPollingRoutine();}};

private:
    //============================== OUTPUT ====================================
    WavetableSine sinewt;   // Simple wavetable sine synth with short decay
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)












public:
    //=========================== Juce System Stuff ============================
    DemoProcessor();
    ~DemoProcessor();
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif
    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;
    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
};
