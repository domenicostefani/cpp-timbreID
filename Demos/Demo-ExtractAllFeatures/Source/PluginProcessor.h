/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - Feature Extractor Plugin

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 10th September 2020

  ==============================================================================
*/

#pragma once

#include "csv_saver.h"
#include "juce_timbreID.h"
#include "postOnsetTimer.h"
#include <JuceHeader.h>

// #define LOG_LATENCY
#define DO_LOG_TO_FILE

#define DO_USE_ATTACKTIME true
#define DO_USE_BARKSPECBRIGHTNESS true
#define DO_USE_BARKSPEC true
#define DO_USE_BFCC true
#define DO_USE_CEPSTRUM true
#define DO_USE_MFCC true
#define DO_USE_PEAKSAMPLE true
#define DO_USE_ZEROCROSSING true

#define BLOCK_SIZE 64
#define FRAME_SIZE 4
#define FRAME_INTERVAL 2
#define ZEROPADS 2

#define MEASURED_ONSET_DETECTION_DELAY_MS 7.7066666667f

/**
 */
class DemoProcessor : public AudioProcessor,
                      public tid::aubio::Onset<float>::Listener,
                      private juce::AudioProcessorValueTreeState::Listener
{
  public:
    float POST_ONSET_DELAY_MS = -1.0f;

    double sampleRate = -1;
    short samplesPerBlock = -1;

    //===================== ONSET DETECTION PARAMS =============================

    // More about the parameter choice at
    // https://github.com/domenicostefani/aubioonset-performanceanalysis/blob/main/report/complessive/Aubio_Study.pdf
    //
    const unsigned int AUBIO_WINDOW_SIZE = 256;
    const unsigned int AUBIO_HOP_SIZE = 64;
    const float ONSET_THRESHOLD = 1.21f;
    const float ONSET_MINIOI = 0.110f; // 110 ms debounce
    const float SILENCE_THRESHOLD = -53.0f;
    const tid::aubio::OnsetMethod ONSET_METHOD = tid::aubio::OnsetMethod::mkl;
    const bool DISABLE_ADAPTIVE_WHITENING = true; // remember to implement in initialization module

    //========================= ONSET DETECTION ================================
    tid::aubio::Onset<float> aubioOnset{AUBIO_WINDOW_SIZE, AUBIO_HOP_SIZE,    ONSET_THRESHOLD,
                                        ONSET_MINIOI,      SILENCE_THRESHOLD, ONSET_METHOD};
    void onsetDetected(tid::aubio::Onset<float> *);
    // double lastOnsetDetectionTime = -1.0;
    long unsigned int lastOnsetDetectionTime = 0;

    PostOnsetTimer postOnsetTimer;
    void onsetDetectedRoutine();

    /**    Feature Vector    **/
    static const unsigned int CSV_FLOAT_PRECISION = 8;

#ifdef WINDOWED_FEATURE_EXTRACTORS
    static WFE::FeatureExtractors<DEFINED_WINDOW_SIZE, DO_USE_ATTACKTIME, DO_USE_BARKSPECBRIGHTNESS, DO_USE_BARKSPEC,
                                  DO_USE_BFCC, DO_USE_CEPSTRUM, DO_USE_MFCC, DO_USE_PEAKSAMPLE, DO_USE_ZEROCROSSING,
                                  BLOCK_SIZE, FRAME_SIZE, FRAME_INTERVAL, ZEROPADS>
        featexts;
#else
    static FE::FeatureExtractors<DEFINED_WINDOW_SIZE, DO_USE_ATTACKTIME, DO_USE_BARKSPECBRIGHTNESS, DO_USE_BARKSPEC,
                                 DO_USE_BFCC, DO_USE_CEPSTRUM, DO_USE_MFCC, DO_USE_PEAKSAMPLE, DO_USE_ZEROCROSSING>
        featexts;
#endif
    std::vector<std::string> header;

    //============================= SAVE CSV ===================================
    // float oldOnsetMinIoi,oldOnsetThreshold,oldOnsetSilence;
    // std::atomic<float> onsetMinIoi,onsetThreshold,onsetSilence;
    std::atomic<bool> clearAt{false}, writeAt{false};

    static const unsigned int VECTOR_SIZE = featexts.getFeVectorSize();

    SaveToCsv<100000, VECTOR_SIZE, CSV_FLOAT_PRECISION> csvsaver;
    int pluginSampleRate, pluginBlockSize;
    void logInCsvSpecial(std::string messagestr);
    long unsigned int sampleCounter = 0;
    //============================== OTHER =====================================
    /**    Debugging    */
#ifdef DEBUG_WHICH_CHANNEL
    uint32 logCounter = 0; // To debug which channel to use
#endif

    /**    Logging    **/
    /*
     * Since many log messages need to be written from the Audio thread, there
     * is a real-time safe logger that uses a FIFO queue as a ring buffer.

     * Messages are written to the queue with:
     * rtlogger.logInfo(message); // message is a char array of size RealTimeLogger::LogEntry::MESSAGE_LENGTH+1
     * or
     * rtlogger.logInfo(message, timeAtStart, timeAtEnd); // both times can be obtained with
     Time::getHighResolutionTicks();
     *
     * Messages have to be consumed with:
     * rtlogger.pop(le)
     *
     * Log Entries are consumed and written to file with a Juce FileLogger from a
     * low priority thread (timer callback), hence why a FileLogger is used.
    */
    tid::RealTimeLogger rtlogger{"main-app"}; // Real time logger that queues messages

    std::vector<tid::RealTimeLogger *> loggerList = {&rtlogger, aubioOnset.getLoggerPtr()};

#ifdef DO_LOG_TO_FILE
    std::unique_ptr<FileLogger> fileLogger; // Logger that writes RT entries to file SAFELY (Outside rt thread
    std::string LOG_PATH = "/tmp/";
    std::string LOG_FILENAME = "featureExtractor";
    const int64 highResFrequency = Time::getHighResolutionTicksPerSecond();

    /**
     * Timer that calls a function every @interval milliseconds.
     * It is used to consume log entries and write them to file safely
     */
    typedef std::function<void(void)> TimerEventCallback;
    class PollingTimer : public Timer
    {
      public:
        PollingTimer(TimerEventCallback cb)
        {
            this->cb = cb;
        }
        void startLogRoutine()
        {
            Timer::startTimer(this->interval);
        }
        void startLogRoutine(int interval)
        {
            this->interval = interval;
            Timer::startTimer(this->interval);
        }
        void timerCallback() override
        {
            cb();
        }

      private:
        TimerEventCallback cb;
        int interval = 500; // Half second default polling interval
    };

    /** POLLING ROUTINE (called at regular intervals by the timer) **/
    void logPollingRoutine()
    {
        /** INITIALIZE THE FILELOGGER IF NECESSARY **/
        if (!this->fileLogger)
            this->fileLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(
                LOG_PATH, LOG_FILENAME, tIDLib::LOG_EXTENSION, "Feature Extractor"));

        tid::RealTimeLogger::LogEntry le;
        for (tid::RealTimeLogger *loggerpointer : this->loggerList) // Cycle through all loggers subscribed to the list
        {
            std::string moduleName = loggerpointer->getName();
            while (loggerpointer->pop(le)) // Continue if there are log entries in the rtlogger queue
            {
                /** CONSUME RTLOGGER ENTRIES AND WRITE THEM TO FILE **/
                std::string msg = moduleName + "\t|\t";
                msg += le.message;
                double start, end, diff;
                start = (le.timeAtStart * 1.0) / this->highResFrequency;
                end = (le.timeAtEnd * 1.0) / this->highResFrequency;
                diff = end - start;
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
    PollingTimer pollingTimer{[this] { logPollingRoutine(); }};
#endif

    /**    This atomic is used to update the onset LED in the GUI **/
    std::atomic<bool> onsetMonitorState{false};

    std::atomic<unsigned int> onsetCounterAtomic{0};

    enum class StorageState
    {
        idle,
        store
    };

    std::atomic<StorageState> storageState{StorageState::idle};

#ifdef LOG_LATENCY
    // Latency log

    int latencyCounter = 0, latencyLogPeriod = 1000;
#endif

    const juce::String STORESTATE_NAME = "Store State", CLEAR_NAME = "Clear Entries", SAVEFILE_NAME = "Save Csv",
                       STORESTATE_ID = "storestate", CLEAR_ID = "clearentries", SAVEFILE_ID = "savecsv";
    AudioProcessorValueTreeState parameters;
    AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void parameterChanged(const juce::String &parameterID, float newValue);

    std::atomic<bool> clear{false}, savefile{false};

  private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DemoProcessor)

  public:
    //=========================== Juce System Stuff ============================
    DemoProcessor();
    ~DemoProcessor();
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
#ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported(const BusesLayout &layouts) const override;
#endif
    void processBlock(AudioBuffer<float> &, MidiBuffer &) override;
    AudioProcessorEditor *createEditor() override;
    bool hasEditor() const override;
    const String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const String getProgramName(int index) override;
    void changeProgramName(int index, const String &newName) override;
    void getStateInformation(MemoryBlock &destData) override;
    void setStateInformation(const void *data, int sizeInBytes) override;
};
