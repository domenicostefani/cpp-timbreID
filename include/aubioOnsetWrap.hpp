/*

Aubio Onset Wrapper - Bridge-header between Juce and the Onset detector from the
Aubio library
More informations at https://aubio.org/

Porting for the cpp-TimbreVST library
Author: Domenico Stefani  (domenico.stefani96@gmail.com)
Date: 25th August 2020

*/
#pragma once

#include <aubio.h>

namespace tid   /* TimbreID namespace*/
{
namespace aubio
{

/**
 * Onset method
 * More info at https://aubio.org/manpages/latest/aubioonset.1.html
*/
enum OnsetMethod
{
    defaultMethod = 0,  //!< Default distance, currently hfc
    energy,             //!< Energy based distance. This function calculates the
                        //!< local energy of the input spectral frame.
    hfc,                //!< High-Frequency content. This method computes the High
                        //!< Frequency Content (HFC) of the input spectral frame.
                        //!< The resulting function is efficient at detecting
                        //!< percussive onsets.
    complex,            //!< Complex domain onset detection function.
                        //!< This function uses information both in frequency and in
                        //!< phase to determine changes in the spectral content that
                        //!< might correspond to musical onsets. It is best suited
                        //!< for complex signals such as polyphonic recordings.
    phase,              //!< Phase based onset detection function.
                        //!< This function uses information both in frequency and in
                        //!< phase to determine changes in the spectral content that
                        //!< might correspond to musical onsets. It is best suited
                        //!< for complex signals such as polyphonic recordings.
    specdiff,           //!< Spectral difference onset detection function.
    kl,                 //!< Kulback-Liebler onset detection function
    mkl,                //!< Modified Kulback-Liebler onset detection function
    specflux            //!< Spectral flux
};


template <typename SampleType>
class Onset
{
public:
    Onset()
    {
        this->analysisWindowSize = 1024;
        this->hopSize = 256;
        this->onsetMethod = OnsetMethod::defaultMethod;
        this->onset_threshold = 0.0;
        this->onset_minioi = 0.0;
    }

    Onset(unsigned long int windowSize, uint_t hopSize, OnsetMethod onsetMethod = OnsetMethod::defaultMethod)
    {
        this->onsetMethod = onsetMethod;
        this->analysisWindowSize = windowSize;
        this->hopSize = hopSize;
        this->onsetMethod = onsetMethod;
        this->onset_threshold = 0.0;
        this->onset_minioi = 0.0;
    }

    Onset(unsigned long int windowSize, uint_t hopSize, SampleType onset_threshold, SampleType onset_minioi, OnsetMethod onsetMethod = OnsetMethod::defaultMethod)
    {
        this->onsetMethod = onsetMethod;
        this->analysisWindowSize = windowSize;
        this->hopSize = hopSize;
        this->onset_threshold = onset_threshold;
        this->onset_minioi = onset_minioi;
    }

    Onset(unsigned long int windowSize, uint_t hopSize, SampleType onset_threshold, SampleType onset_minioi, SampleType silenceThreshold, OnsetMethod onsetMethod = OnsetMethod::defaultMethod)
    {
        this->onsetMethod = onsetMethod;
        this->analysisWindowSize = windowSize;
        this->hopSize = hopSize;
        this->onset_threshold = onset_threshold;
        this->onset_minioi = onset_minioi;
        this->silence_threshold = silenceThreshold;
    }

    ~Onset(){}

    /** Creates a copy of another Onset module. */
    Onset (const Onset&) = default;

    /** Move constructor */
    Onset (Onset&&) = default;

    void prepare(unsigned long int sampleRate, unsigned int blockSize)
    {
        this->blockSize = blockSize;
        resizeBuffer();
        logger.logInfo("Preparing onset detector");
        const bool VERBOSE = false;

        o = new_aubio_onset (this->getStringMethod().c_str(), (uint_t)this->analysisWindowSize, this->hopSize, (uint_t)sampleRate);
        if (o == NULL)
        {
            throw std::logic_error("Aubio Onset initialization failed");
        }

        if (onset_threshold != 0.)
        {
            if(VERBOSE) logger.logInfo("Setting onset threshold to " + std::to_string(onset_threshold));
            aubio_onset_set_threshold (o, onset_threshold);
        }
        if (silence_threshold != -90.)
        {
            if(VERBOSE) logger.logInfo("Setting silence threshold to " + std::to_string(silence_threshold));
            aubio_onset_set_silence (o, silence_threshold);
        }
        if (onset_minioi != 0.)
        {
            if(VERBOSE) logger.logInfo("Setting minimum inter-onset interval to " + std::to_string(onset_minioi));
            aubio_onset_set_minioi_s (o, onset_minioi);
        }

        if(VERBOSE)
        {
            logger.logInfo("Sample rate: "+std::to_string(sampleRate)+"Hz");
            logger.logInfo("onset method: " + getStringMethod() + ", ");
            logger.logInfo("buffer_size: "+std::to_string(analysisWindowSize)+", ");
            logger.logInfo("hop_size: "+std::to_string(hopSize)+", ");
            logger.logInfo("silence: "+std::to_string(aubio_onset_get_silence(o))+", ");
            logger.logInfo("threshold: "+std::to_string(aubio_onset_get_threshold(o))+", ");
            logger.logInfo("awhitening: "+std::to_string(aubio_onset_get_awhitening(o))+", ");
            logger.logInfo("compression "+std::to_string(aubio_onset_get_compression(o)));
        }

        onset_fvec = new_fvec (1);
        input_fvec = new_fvec (hopSize);
    }

    void reset()
    {
        logger.logInfo("Reset and release memory");
        // destroy the input vector
        if(input_fvec)
        {
            del_fvec(input_fvec);
            input_fvec = nullptr;
        }
        if(o)
        {
            del_aubio_onset (o);
            o = nullptr;
        }
        if(onset_fvec)
        {
            del_fvec (onset_fvec);
            onset_fvec = nullptr;
        }
    }

    template <typename OtherSampleType>
    void store (AudioBuffer<OtherSampleType>& buffer, short channel)
    {
        static_assert (std::is_same<OtherSampleType, SampleType>::value,
                       "The sample-type of the Onset module must match the sample-type supplied to this store callback");
        short numChannels = buffer.getNumChannels();
        jassert(channel < numChannels);
        jassert(channel >= 0);
        jassert(numChannels > 0);

        if(channel < 0 || channel >= numChannels)
            throw std::invalid_argument("Channel index has to be between 0 and " + std::to_string(numChannels));

        perform(buffer.getReadPointer(channel), buffer.getNumSamples());
    }

    unsigned int getHopSize()
    {
        return this->hopSize;
    }

    SampleType getSilenceThreshold()
    {
        return this->silence_threshold;
    }

    OnsetMethod getOnsetMethod()
    {
        return this->getOnsetMethod;
    }

    std::string getStringOnsetMethod()
    {
        return getStringMethod();
    }

    SampleType getOnsetThreshold()
    {
        return this->onset_threshold;
    }

    SampleType getOnsetMinInterOnsetInterval()
    {
        return this->onset_minioi;
    }

    unsigned long int getWindowSize()
    {
        return this->analysisWindowSize;
    }

    /*-----------------------------*/

    /**
       Used to receive callbacks when an onset is detected.

       @see Onset::addListener, Onset::removeListener
    */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Called when the onset is detected. */
        virtual void onsetDetected (Onset *) = 0;
    };

    /** Registers a listener to receive events when onsets are detected.
        If the listener is already registered, this will not register it again.
        @see removeListener
    */
    void addListener (Listener* newListener)
    {
        onsetListeners.add(newListener);
    }

    /** Removes a previously-registered Onset listener
        @see addListener
    */
    void removeListener (Listener* listener)
    {
        onsetListeners.remove(listener);
    }

    void setOnsetMethod(OnsetMethod method){
        this->onsetMethod = method;
    }

    void setOnsetThreshold(float onset_threshold)
    {
        this->onset_threshold = onset_threshold;
    }
    void setSilenceThreshold(float silence_threshold)
    {
        this->silence_threshold = silence_threshold;
    }
    void setOnsetMinioi(float onset_minioi)
    {
        this->onset_minioi = onset_minioi;
    }

private:
    void resizeBuffer()
    {
        signalBuffer.resize(this->hopSize, this->blockSize);
    }

    void perform(const SampleType* input, size_t n)
    {
        //#define VERBOSE_PERFORM
        // write new block to end of signal buffer.
        for(unsigned long int i=0; i<n; ++i)
            signalBuffer[this->dspTick+i] = input[i];

        int numSamples = this->dspTick + n;
        jassert(numSamples < this->blockSize + this->hopSize);
        int turns = numSamples/hopSize;

       #ifdef VERBOSE_PERFORM
        logger.logInfo("numSamples: " + std::to_string(numSamples) + " turns: " + std::to_string(turns) + "\n");
       #endif

        bool onsetDetected = false;
        // Analyze buffer in hopSize increments
        for (int j = 0; j < turns; ++j)
        {
            int firstIndex = (j * hopSize);

           #ifdef VERBOSE_PERFORM
            logger.logInfo("Analyzing section [" + std::to_string(firstIndex) + "," + std::to_string(lastIndex) + ")\n"); //TODO: remove
           #endif

            for(unsigned long int i=0; i<hopSize; ++i)
                fvec_set_sample(input_fvec, this->signalBuffer[firstIndex+i],firstIndex+i);
            aubio_onset_do (o, input_fvec, onset_fvec);
            smpl_t is_onset = fvec_get_sample(onset_fvec, 0);

            if (is_onset)
                onsetDetected = true; //One onset in the frame is sufficient to send signal
            this->dspTick = 0;
        }

        // Buffer for next round
        {
            int remaining = numSamples % hopSize;
            if (remaining)
            {
                int firstIndex = (int) (n / hopSize) * hopSize;

               #ifdef VERBOSE_PERFORM
                logger.logInfo("Buffering section [" + std::to_string(firstIndex) + "," + std::to_string(lastIndex) + ")\n"); //Todo remove
               #endif

                for(int i=0; i < signalBuffer.size(); ++i)
                {
                    if(i<remaining)
                        signalBuffer[i] = signalBuffer[firstIndex+i];
                    else
                        signalBuffer[i] = 0.0f;
                }
                this->dspTick = remaining;
            }
        }
        if (onsetDetected)
            outputOnset();
    }

    void outputOnset()
    {
        onsetListeners.call([this] (Listener& l) { l.onsetDetected (this); });
    }

    std::string getStringMethod()
    {
        std::string res;

        switch(this->onsetMethod)
        {
            case OnsetMethod::defaultMethod :
                res = "default";
                break;
            case OnsetMethod::energy :
                res = "energy";
                break;
            case OnsetMethod::hfc :
                res = "hfc";
                break;
            case OnsetMethod::complex :
                res = "complex";
                break;
            case OnsetMethod::phase :
                res = "phase";
                break;
            case OnsetMethod::specdiff :
                res = "specdiff";
                break;
            case OnsetMethod::kl :
                res = "kl";
                break;
            case OnsetMethod::mkl :
                res = "mkl";
                break;
            case OnsetMethod::specflux :
                res = "specflux";
                break;
            default:
                res = "default";
                break;
        }


        return res;
    }

    ListenerList<Listener> onsetListeners;

    aubio_onset_t *o = nullptr;
    fvec_t * input_fvec; //input vector
    fvec_t *onset_fvec;       //output vector/value

    // general stuff
    uint_t hopSize;
    smpl_t silence_threshold = -90.;

    // onset stuff
    OnsetMethod onsetMethod = OnsetMethod::defaultMethod;
    smpl_t onset_threshold = 0.0; // will be set if != 0.
    smpl_t onset_minioi = 0.0; // will be set if != 0.

    std::vector<SampleType> signalBuffer;

    unsigned long int analysisWindowSize = 2048;

    unsigned long int dspTick = 0;
    unsigned int blockSize = 0;

    Log logger{"aubio-onset_"};
};

} // namespace Aubio
} // namespace tid
