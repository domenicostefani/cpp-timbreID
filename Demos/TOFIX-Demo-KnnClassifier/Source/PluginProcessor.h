/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - bark Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"

#define USE_AUBIO_ONSET //If this commented, the bark onset detector is used, otherwise the aubio onset module is used

//==============================================================================
/**
*/
class DemoProcessor : public AudioProcessor,
                      public Timer,
#ifdef USE_AUBIO_ONSET
                      public tid::aubio::Onset<float>::Listener
#else
                      public tid::Bark<float>::Listener
#endif
{
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

    const unsigned int WINDOW_SIZE = 1024;
    const float BARK_SPACING = 0.5;
    //=================== Onset module initialization ==========================

   #ifdef USE_AUBIO_ONSET
    /**    Set initial parameters      **/
    const unsigned int HOP = 128;
    const float ONSET_THRESHOLD = 0.0f;
    const float ONSET_MINIOI = 0.020f;  //20 ms debounce
    const float SILENCE_THRESHOLD = -80.0f;
    const tid::aubio::OnsetMethod ONSET_METHOD = tid::aubio::OnsetMethod::defaultMethod;

    tid::aubio::Onset<float> aubioOnset{WINDOW_SIZE,HOP,ONSET_THRESHOLD,ONSET_MINIOI,SILENCE_THRESHOLD,ONSET_METHOD};

    void onsetDetected (tid::aubio::Onset<float> *);
   #else
    /**    Set initial parameters      **/
    const unsigned int HOP = 128;

    /**    Initialize the onset detector      **/
    tid::Bark<float> bark{WINDOW_SIZE, HOP, BARK_SPACING};
    void onsetDetected (tid::Bark<float>* bark);
   #endif
//     void onsetDetectedRoutine();

    /**    Initialize the modules      **/
    tid::Bfcc<float> bfcc{WINDOW_SIZE, BARK_SPACING};
    tid::KNNclassifier knn;

    /**    This atomic is used to update the onset LED in the GUI **/
    std::atomic<bool> onsetMonitorState{false};

    std::atomic<unsigned int> matchAtomic{0};
    std::atomic<float> distAtomic{-1.0f};

//     std::atomic<bool> onsetWasDetected{false};

    /**    Only the first n features of bfcc are used  **/
    int featuresUsed = 50;

    enum class CState {
        idle,
        train,
        classify
    };

    CState classifierState = CState::idle;

    void timerCallback();

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

//     std::vector<tid::RealTimeLogger*> loggerList = {&rtlogger, aubioOnset.getLoggerPtr()};

//     std::unique_ptr<FileLogger> fileLogger; // Logger that writes RT entries to file SAFELY (Outside rt thread
//     std::string LOG_PATH = "/tmp/";
//     std::string LOG_FILENAME = "KnnClassifierDemo";
//     const int64 highResFrequency = Time::getHighResolutionTicksPerSecond();

//     /**
//      * Timer that calls a function every @interval milliseconds.
//      * It is used to consume log entries and write them to file safely
//     */
//     typedef std::function<void (void)> TimerEventCallback;
//     class PollingTimer : public Timer
//     {
//     public:
//         PollingTimer(TimerEventCallback cb) { this->cb = cb; }
//         void startLogRoutine() { Timer::startTimer(this->interval); }
//         void startLogRoutine(int interval) { this->interval = interval; Timer::startTimer(this->interval); }
//         void timerCallback() override { cb(); }
//     private:
//         TimerEventCallback cb;
//         int interval = 500; // Half second default polling interval
//     };

//     /** POLLING ROUTINE (called at regular intervals by the timer) **/
//     void logPollingRoutine()
//     {
//         /** INITIALIZE THE FILELOGGER IF NECESSARY **/
//         if (!this->fileLogger)
//             this->fileLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(LOG_PATH, LOG_FILENAME, tIDLib::LOG_EXTENSION, "Knn Classifier"));

//         tid::RealTimeLogger::LogEntry le;
//         for(tid::RealTimeLogger* loggerpointer : this->loggerList) // Cycle through all loggers subscribed to the list
//         {
//             std::string moduleName = loggerpointer->getName();
//             while (loggerpointer->pop(le))  // Continue if there are log entries in the rtlogger queue
//             {
//                 /** CONSUME RTLOGGER ENTRIES AND WRITE THEM TO FILE **/
//                 std::string msg = moduleName + "\t|\t";
//                 msg += le.message;
//                 double start,end,diff;
//                 start = (le.timeAtStart * 1.0) / this->highResFrequency;
//                 end = (le.timeAtEnd * 1.0) / this->highResFrequency;
//                 diff = end-start;
//                 if (start != 0)
//                 {
//                     msg += " | start: " + std::to_string(start);
//                     msg += " | end: " + std::to_string(end);
//                     msg += " | difference: " + std::to_string(diff) + "s";
//                 }
//                 fileLogger->logMessage(msg);
//             }
//         }
//     }

//     /** Instantiate polling timer and pass callback **/
//     PollingTimer pollingTimer{[this]{logPollingRoutine();}};

private:

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DemoProcessor)
};
