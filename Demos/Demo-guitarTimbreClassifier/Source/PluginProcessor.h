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
#include "squaredSineWavetable.h"

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



#include "postOnsetTimer.h"
#include "ClassifierFifoQueue.h"    // Lock-free queue that can store whole std::array elements
#include <thread>

#define USE_AUBIO_ONSET //If this commented, the bark onset detector is used, otherwise the aubio onset module is used
#define MEASURE_COMPUTATION_LATENCY
// #define DEBUG_WHICH_CHANNEL
#define PARALLEL_CLASSIFICATION // If defined, classification is performed on a separate thread
// #define FAST_MODE_1 // In fast mode 1, logs and other operations are suppressed.
#define DEBUG_WITH_SPIKE    // if defined, a spike is added to the signal output at both the time of onset detection and that of classification termination 

#define SEND_OSC
#ifdef SEND_OSC
 #define OSC_TARGET_IP "192.168.42.38"
//  #define OSC_TARGET_IP "192.168.42.38"
 #define OSC_TARGET_PORT 9001
#endif

namespace CData{

const std::size_t N_FEATURES = 180;       // Number of audio features
const int N_CLASSES = 8;                  // Number of classes for the neural network
const int CLASSIFICATION_FIFO_BUFFER = 5; // Size of the ring buffers

#ifdef PARALLEL_CLASSIFICATION

struct ClassificationData{
    // In and Out buffers for parallel classification
    ClassifierQueue<N_FEATURES,CLASSIFICATION_FIFO_BUFFER> featureBuffer;   // Audio thread --(featurevector)-> Classification thread
    ClassifierQueue<N_CLASSES,CLASSIFICATION_FIFO_BUFFER> predictionBuffer;  // Audio thread <--(predictions)--- Classification thread
    // Element types for both queues
    using features_t = ClassifierQueue<N_FEATURES,CLASSIFICATION_FIFO_BUFFER>::ElementType;
    using predictions_t = ClassifierQueue<N_CLASSES,CLASSIFICATION_FIFO_BUFFER>::ElementType;
    // Run flag, to start and stop the classification thread
    std::atomic<bool> run;
    std::atomic<int> i;    
};

class ClassificationThread : public juce::Thread
{
public:
    ClassificationThread (ClassificationData* cdata, ClassifierPtr* tclassifier)
        : Thread ("ClassificationThread")
    {
        this->cdata = cdata;
        this->tclassifier = tclassifier;

       #ifdef SEND_OSC
        if (! this->sender.connect (OSC_TARGET_IP, OSC_TARGET_PORT))
            showConnectionErrorMessage ("Error: could not connect to UDP port.");
       #endif

        startThread (6);
    }

    ~ClassificationThread() override
    {
        stopThread (2000); // allow the thread 2 seconds to stop cleanly - should be plenty of time.
    }

    void run() override
    {
        while (! threadShouldExit())
        {
            if(cdata == nullptr) throw std::logic_error("Invalid ClassificationData pointer;");
            if(tclassifier == nullptr) throw std::logic_error("Invalid timbreClassifier pointer;");


            static ClassificationData::features_t readFeatures;
            if (cdata->featureBuffer.read(readFeatures)) // Read from feature queue
            {
                static ClassificationData::predictions_t towritePredictions;
                classify(*tclassifier,readFeatures,towritePredictions);
               #ifdef SEND_OSC
                if (! this->sender.send ("/techclassifier/class", (float) towritePredictions[0],
                                                                  (float) towritePredictions[1],
                                                                  (float) towritePredictions[2],
                                                                  (float) towritePredictions[3],
                                                                  (float) towritePredictions[4],
                                                                  (float) towritePredictions[5],
                                                                  (float) towritePredictions[6],
                                                                  (float) towritePredictions[7]))
                    showConnectionErrorMessage ("Error: could not send OSC message.");
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
   #ifdef SEND_OSC
    void showConnectionErrorMessage (const juce::String& messageText)
    {
        std::cerr << messageText.toStdString() << std::endl;
    }
    juce::OSCSender sender;
   #endif
    ClassificationData* cdata = nullptr;
    ClassifierPtr* tclassifier = nullptr;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClassificationThread)
};

#endif

}


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
    std::array<float, CData::N_FEATURES> featureVector;
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
    static ClassifierPtr timbreClassifier; // Tensorflow interpreter

    const std::string MODEL_NAME = "model1-best";
    // const std::string MODEL_NAME = "model2-nobatchnorm";
    
   #ifdef USE_TFLITE
    const std::string MODEL_SUBFOLDER = "tflite";
    const std::string MODEL_EXTENSION = "tflite";
   #endif

   #if defined(USE_RTNEURAL_CTIME) ||  defined(USE_RTNEURAL_RTIME)
    const std::string MODEL_SUBFOLDER = "rtneural";
    const std::string MODEL_EXTENSION = "json";
   #endif

   #ifdef USE_ONNX
    const std::string MODEL_SUBFOLDER = "onnx";
    const std::string MODEL_EXTENSION = "onnx";
   #endif

   #ifdef USE_TORCHSCRIPT
    const std::string MODEL_SUBFOLDER = "torchscript";
    const std::string MODEL_EXTENSION = "pt";
   #endif


    const std::string MODEL_PATH =  "/udata/neural_net_models/"+MODEL_SUBFOLDER+"/"+MODEL_NAME+"."+MODEL_EXTENSION+"";

    std::array<float, CData::N_CLASSES> classificationOutputVector;


   #ifdef PARALLEL_CLASSIFICATION
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
  #endif

private:
    //============================== OUTPUT ====================================
    WavetableSine sinewt;   // Simple wavetable sine synth with short decay
    SquaredSine squaredsinewt;   // Simple wavetable sine synth with short decay
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
