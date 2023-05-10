/*
  ==============================================================================
     Plugin Processor

     DEMO PROJECT - TimbreID - Guitar Timbre Classifier

     Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
     Date: 24th August 2020
  ==============================================================================
*/

#pragma once

#include "juce_timbreID.h"
#include "squaredSineWavetable.h"
#include "wavetableSine.h"
#include <JuceHeader.h>

#include "readJsonConfig.h"

#define WINDOWED_FEATURE_EXTRACTION

#ifdef USE_TFLITE_LEGACY
#include "liteclassifier.h"
#endif

#ifdef USE_TFLITE
#include "tflitewrapper.h"
#endif

#ifdef USE_RTNEURAL_CTIME
#include "rtneuralwrapper.h"
#endif
#ifdef USE_RTNEURAL_RTIME
#include "rtneuralwrapper.h"
#endif

#ifdef USE_ONNX
#include "onnxwrapper.h"
#endif

#ifdef USE_TORCHSCRIPT
#include "torch-classifier-lib.h"
#endif

#define DO_USE_ATTACKTIME true
#define DO_USE_BARKSPECBRIGHTNESS true
#define DO_USE_BARKSPEC true
#define DO_USE_BFCC true
#define DO_USE_CEPSTRUM true
#define DO_USE_MFCC true
#define DO_USE_PEAKSAMPLE true
#define DO_USE_ZEROCROSSING true
// - Windowing Specs
#define BLOCK_SIZE 64
#define FRAME_SIZE 4
#define FRAME_INTERVAL 2
#define ZEROPADS 2

#define MEASURED_ONSET_DETECTION_DELAY_MS 7.7066666667f

#include "ClassifierArrayFifoQueue.h"  // Lock-free queue that can store whole std::array elements
#include "ClassifierVectorFifoQueue.h" // Same but with vectors, for runtime-defined size
#include "postOnsetTimer.h"
#include <thread>

#define USE_AUBIO_ONSET // If this commented, the bark onset detector is used, otherwise the aubio onset module is used
#define MEASURE_COMPUTATION_LATENCY
// #define DEBUG_WHICH_CHANNEL

// ! This is defined in the Projucer configuration
// #define SEQUENTIAL_CLASSIFICATION // If defined, classification is performed SEQUENTIALLY on the audio thread, this
// might be bad This is defined in some of the projucer's configs to test small nn models
// ! This is defined in the Projucer configuration

// #define FAST_MODE_1 // In fast mode 1, logs and other operations are suppressed.
// #define DEBUG_WITH_SPIKE    // if defined, a spike is added to the signal output at both the time of onset detection
// and that of classification termination

#define STOP_OSC

namespace CData
{

const std::size_t N_FEATURES = 180;       // Number of audio features
const int N_CLASSES = 8;                  // Number of classes for the neural network
const int CLASSIFICATION_FIFO_BUFFER = 5; // Size of the ring buffers

#ifndef SEQUENTIAL_CLASSIFICATION

struct ClassificationData
{
    // In and Out buffers for parallel classification
    ClassifierVectorQueue<CLASSIFICATION_FIFO_BUFFER>
        featureBuffer; // Audio thread --(featurevector)-> Classification thread
    ClassifierArrayQueue<N_CLASSES, CLASSIFICATION_FIFO_BUFFER>
        predictionBuffer; // Audio thread <--(predictions)--- Classification thread
    // Element types for both queues
    using features_t = ClassifierVectorQueue<CLASSIFICATION_FIFO_BUFFER>::ElementType;
    using predictions_t = ClassifierArrayQueue<N_CLASSES, CLASSIFICATION_FIFO_BUFFER>::ElementType;
    // Run flag, to start and stop the classification thread
    std::atomic<bool> run;
    std::atomic<int> i;
    std::atomic<size_t> nRows = -1, nCols = -1;
};

class ClassificationThread : public juce::Thread
{
  public:
    ClassificationThread(ClassificationData *cdata, ClassifierPtr *tclassifier) : Thread("ClassificationThread")
    {
        this->cdata = cdata;
        this->tclassifier = tclassifier;

        startThread(6);
    }

    ~ClassificationThread() override
    {
        stopThread(2000); // allow the thread 2 seconds to stop cleanly - should be plenty of time.
    }

#ifndef STOP_OSC
    void connectOSC(std::string osc_target_ip, int osc_target_port)
    {
        if (!this->sender.connect(osc_target_ip, osc_target_port))
            showConnectionErrorMessage("Error: could not connect to UDP port.");
        else
            this->isOSCconnected = true;
    }

    void setOSCmessage(std::string osc_message)
    {
        this->oscMessage = osc_message;
    }

    void setFullOSCmessage(bool fullOSCmessage)
    {
        this->full_message_type = fullOSCmessage;
    }
#endif

    void set1DinputSize(size_t featureNumber)
    {
        this->cdata->featureBuffer.reserveVectorSize(featureNumber);
    }

    void set2DinputSize(size_t nRows, size_t nCols)
    {
        this->cdata->nRows = nRows;
        this->cdata->nCols = nCols;
        this->cdata->featureBuffer.reserveVectorSize(nRows * nCols);
    }

    void run() override
    {
        while (!threadShouldExit())
        {
            if (cdata == nullptr)
                throw std::logic_error("Invalid ClassificationData pointer;");
            if (tclassifier == nullptr)
                throw std::logic_error("Invalid timbreClassifier pointer;");

            static ClassificationData::features_t readFeatures;
            if (cdata->featureBuffer.read(readFeatures)) // Read from feature queue
            {
                static ClassificationData::predictions_t towritePredictions;
                // 1D input
                // classify(*tclassifier,&(readFeatures[0]), readFeatures.size(),&(towritePredictions[0]),
                // towritePredictions.size());

                // 2D input
                if (cdata->nRows == -1 || cdata->nCols == -1)
                    throw std::logic_error("Invalid input size");
                classifyFlat2D(*tclassifier, readFeatures.data(), cdata->nRows, cdata->nCols, towritePredictions.data(),
                               towritePredictions.size(), false);
#ifndef STOP_OSC
                if (isOSCconnected)
                {
                    if (full_message_type)
                    {
                        if (!this->sender.send(this->oscMessage.c_str(), (float)towritePredictions[0],
                                               (float)towritePredictions[1], (float)towritePredictions[2],
                                               (float)towritePredictions[3], (float)towritePredictions[4],
                                               (float)towritePredictions[5], (float)towritePredictions[6],
                                               (float)towritePredictions[7]))
                            showConnectionErrorMessage("Error: could not send OSC message.");
                    }
                    else
                    {
                        float maxclass = 0;
                        if (towritePredictions[1] > towritePredictions[maxclass])
                            maxclass = 1;
                        if (towritePredictions[2] > towritePredictions[maxclass])
                            maxclass = 2;
                        if (towritePredictions[3] > towritePredictions[maxclass])
                            maxclass = 3;
                        if (towritePredictions[4] > towritePredictions[maxclass])
                            maxclass = 4;
                        if (towritePredictions[5] > towritePredictions[maxclass])
                            maxclass = 5;
                        if (towritePredictions[6] > towritePredictions[maxclass])
                            maxclass = 6;
                        if (towritePredictions[7] > towritePredictions[maxclass])
                            maxclass = 7;

                        if (!this->sender.send(this->oscMessage.c_str(), (int)maxclass,
                                               (float)towritePredictions[maxclass]))
                            showConnectionErrorMessage("Error: could not send OSC message.");
                    }
                }
#endif
                cdata->predictionBuffer.write(towritePredictions);
            }

            // wait(1); // Wait to avoid starving (Busy waiting)

            // Possible alternative for quicker polling
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(500us);
        }
    }

  private:
#ifndef STOP_OSC
    bool isOSCconnected = false;
    void showConnectionErrorMessage(const juce::String &messageText)
    {
        std::cerr << messageText.toStdString() << std::endl;
    }
    juce::OSCSender sender;
    std::string oscMessage = "/";
    bool full_message_type = true;
#endif
    ClassificationData *cdata = nullptr;
    ClassifierPtr *tclassifier = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassificationThread)
};

#endif

} // namespace CData

class DemoProcessor : public AudioProcessor,
#ifdef USE_AUBIO_ONSET
                      public tid::aubio::Onset<float>::Listener
#else
                      public tid::Bark<float>::Listener
#endif
{
  public:
    float POST_ONSET_DELAY_MS = -1.0f;
    //===================== ONSET DETECTION PARAMS =============================

#ifdef USE_AUBIO_ONSET
    //
    // More about the parameter choice at
    // https://github.com/domenicostefani/aubioonset-performanceanalysis/blob/main/report/complessive/Aubio_Study.pdf
    //
    const unsigned int AUBIO_WINDOW_SIZE = 256;
    const unsigned int AUBIO_HOP_SIZE = 64;
    const float ONSET_THRESHOLD = 1.21f;
    const float ONSET_MINIOI = 0.02f; // 20 ms debounce
    const float SILENCE_THRESHOLD = -53.0f;
    const tid::aubio::OnsetMethod ONSET_METHOD = tid::aubio::OnsetMethod::mkl;
    const bool DISABLE_ADAPTIVE_WHITENING = true; // remember to implement in initialization module
#else
    const unsigned int BARK_WINDOW_SIZE = 1024;
    const unsigned int BARK_HOP_SIZE = 128;
#endif

    //==================== FEATURE EXTRACTION PARAMS ===========================
    const unsigned int FEATUREEXT_WINDOW_SIZE = 704; // 11 Blocks of 64samples, 14.66ms
    const float BARK_SPACING = 0.5f;
    const float BARK_BOUNDARY = 8.5f;
    const float MEL_SPACING = 100;

    //========================= ONSET DETECTION ================================
#ifdef USE_AUBIO_ONSET
    tid::aubio::Onset<float> aubioOnset{AUBIO_WINDOW_SIZE, AUBIO_HOP_SIZE,    ONSET_THRESHOLD,
                                        ONSET_MINIOI,      SILENCE_THRESHOLD, ONSET_METHOD};
    void onsetDetected(tid::aubio::Onset<float> *);
#else
    /**    Initialize the onset detector      **/
    tid::Bark<float> bark{BARK_WINDOW_SIZE, BARK_HOP_SIZE, BARK_SPACING};
    void onsetDetected(tid::Bark<float> *bark);
#endif

    PostOnsetTimer postOnsetTimer;
    void onsetDetectedRoutine();

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
    std::vector<float> featureVector;
    int filteredFeatureMatrix_nrows = -1, filteredFeatureMatrix_ncols = -1;

    bool primeClassifier = true;

    double sampleRate = -1;
    short samplesPerBlock = -1;

    //========================== CLASSIFICATION ================================
    static ClassifierPtr timbreClassifier; // Tensorflow interpreter

    const std::string NN_CONFIG_JSON_PATH = "/udata/phase3experiment.json";
    std::string MODEL_PATH = "";

    std::array<float, CData::N_CLASSES> classificationOutputVector;

#ifndef SEQUENTIAL_CLASSIFICATION
    // Classification is performed on a separate thread
    // Feature vectors are fed to the classifier through a lock-free queue
    // Classification result vectors are fed back to the audio thread through a second lock-free queue

    CData::ClassificationThread classificationThread;
    static CData::ClassificationData classificationData;

    void startClassifierThread();
    void stopClassifierThread();
#endif
    void classificationFinished();

    //============================== OTHER =====================================
#ifndef FAST_MODE_1
    /**    Debugging    */
#ifdef DEBUG_WHICH_CHANNEL
    uint32 logCounter = 0; // To debug which channel to use
#endif
    /**    Profiling    **/
#ifdef MEASURE_COMPUTATION_LATENCY
    double latencyTime = 0, classf_start = 0, classf_end = 0;
    std::chrono::time_point<std::chrono::high_resolution_clock> chrono_start, chrono_end;
    uint64 logProcessblock_counter = 0, logProcessblock_fixedCounter = 0;
    const uint64 logProcessblock_period =
        48000 / 64 * 5; // Log every 5 second the duration of the processblock routine (SampleRate/BlockSize)
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

#ifdef USE_AUBIO_ONSET
    std::vector<tid::RealTimeLogger *> loggerList = {&rtlogger, aubioOnset.getLoggerPtr()};
#else
    std::vector<tid::RealTimeLogger *> loggerList = {&rtlogger, bark.getLoggerPtr()};
#endif
    std::unique_ptr<FileLogger> fileLogger; // Logger that writes RT entries to file SAFELY (Outside rt thread
    std::string LOG_PATH = "/tmp/";
    std::string LOG_FILENAME = "phase3-ExpGuiTecClas-";
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
                LOG_PATH, LOG_FILENAME, tIDLib::LOG_EXTENSION, "Guitar Timbre Classifier"));

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

  private:
    //============================== OUTPUT ====================================
    WavetableSine sinewt;      // Simple wavetable sine synth with short decay
    SquaredSine squaredsinewt; // Simple wavetable sine synth with short decay
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
