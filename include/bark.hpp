/*

bark - Bridge-header between Juce and the TimbreID bark~ module
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 14th April 2020
  (domenico.stefani96@gmail.com)

Porting from bark~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/bark~.c/

**** Original LICENCE disclaimer ahead ****

Copyright 2010 William Brent

bark~ is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

bark~ is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.
 should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#pragma once

#include "tIDLib.hpp"
#include "fftw3.h"

#define DEBUGLOG_SUBFOLDER "tIDLib-bark"
#define DEBUGLOG_FILENAME "debug_log_"
#define MEASURELOG_SUBFOLDER "tIDLib-bark"
#define MEASURELOG_FILENAME "measure_log_"

namespace tid   /* TimbreID namespace*/
{

const float weights_dB[] = {-69.9, -60.4, -51.4, -43.3, -36.6, -30.3, -24.3, -19.5, -14.8, -10.7, -7.5, -4.8, -2.6, -0.8, 0.0, 0.6, 0.5, 0.0, -0.1, 0.5, 1.5, 3.6, 5.9, 6.5, 4.2, -2.6, -10.2, -10.0, -2.8};
const float weights_freqs[] = {20, 25, 31.5, 40, 50, 63, 80, 100, 125, 160, 200, 250, 315, 400, 500, 630, 800, 1000, 1250, 1600, 2000, 2500, 3150, 4000, 5000, 6300, 8000, 10000, 12500};

template <typename SampleType>
class Bark : private Timer
{
public:
    //==========================================================================
    /** Creates a Bark module with default parameters. */
    Bark(){initModule();}

    /**
     * Bark constructor
     * Creates a Bark module specifying an additional parameter
     * @param analysisWindowSize size of the analysis window
    */
    Bark(unsigned long int analysisWindowSize)
    {
        if(analysisWindowSize < tIDLib::MINWINDOWSIZE)
            throw std::invalid_argument("Window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater.");

        this->analysisWindowSize = analysisWindowSize;
        this->hop = analysisWindowSize*0.25;
        this->barkSpacing = tIDLib::BARKSPACINGDEFAULT;
        initModule();
    }

    /**
     * Bark constructor
     * Creates a Bark module specifying additional parameters
     * @param analysisWindowSize size of the analysis window
     * @param hop number of samples are analysed at every period
    */
    Bark(unsigned long int analysisWindowSize, unsigned long int hop)
    {
        if(analysisWindowSize < tIDLib::MINWINDOWSIZE)
            throw std::invalid_argument("Window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater.");
        if(hop < 1)
            throw std::invalid_argument("hop must be greater than 1 sample.");

        this->analysisWindowSize = analysisWindowSize;
        this->hop = hop;
        this->barkSpacing = tIDLib::BARKSPACINGDEFAULT;
        initModule();
    }

    /**
     * Bark constructor
     * Creates a Bark module specifying additional parameters
     * @param analysisWindowSize size of the analysis window
     * @param hop number of samples are analysed at every period
     * @param barkSpacing filter spacing in Barks
    */
    Bark(unsigned long int analysisWindowSize, unsigned long int hop, float barkSpacing)
    {
        if(analysisWindowSize < tIDLib::MINWINDOWSIZE)
            throw std::invalid_argument("Window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater.");
        if(hop < 1)
            throw std::invalid_argument("hop must be greater than 1 sample.");
        if(barkSpacing < tIDLib::MINBARKSPACING || barkSpacing > tIDLib::MAXBARKSPACING)
            throw std::invalid_argument("Bark spacing must be between "+std::to_string(tIDLib::MINBARKSPACING)+" and "+std::to_string(tIDLib::MAXBARKSPACING)+" Barks.");

        this->analysisWindowSize = analysisWindowSize;
        this->hop = hop;
        this->barkSpacing = barkSpacing;
        initModule();
    }

    /** Creates a copy of another Bark module. */
    Bark (const Bark&) = default;

    /** Move constructor */
    Bark (Bark&&) = default;

    /** Free memory used */
    ~Bark(){ freeMem(); }

    //==========================================================================
    /**
     * Initialization of the module
     * Prepares the module to play by clearing buffers and setting audio buffers
     * standard parameters
     * @param sampleRate The sample rate of the buffer
     * @param blockSize The size of the individual audio blocks
    */
    void prepare (unsigned long int sampleRate, unsigned int blockSize) noexcept
    {
        if((sampleRate != this->sampleRate) || (blockSize != this->blockSize))
        {
            this->sampleRate = sampleRate;
            this->blockSize = blockSize;
            resizeBuffer();
        }
        reset();
    }

    /**
     * Resets the processing pipeline.
     * It empties the analysis buffer
    */
    void reset() noexcept
    {
        std::fill(signalBuffer.begin(), signalBuffer.end(), SampleType{0});
    }
    //==========================================================================
    /**
     * Container class for growth data
     * Used to return the growth values to the caller.
     * If the elements returned are valid, the getters will return them, otherwise
     * nullptr is returned
    */
    class GrowthData
    {
    public:
        void setData(const std::vector<float>& growth, float totalGrowth)
        {
            this->growth = growth;
            this->totalGrowth = totalGrowth;
            this->validData = true;
        }
        std::vector<float>* getGrowth()
        {
            if(validData)
                return &growth;
            return nullptr;
        }
        float* getTotalGrowth()
        {
            if(validData)
                return &totalGrowth;
            return nullptr;
        }
    private:
        bool validData = false;
        std::vector<float> growth;
        float totalGrowth;
    };
    //==========================================================================

    /**
     * Stores an audio block into the buffer for onset detection
     * It stores the audio block provided into a buffer where onset detection is
     * performed every hop samples.
     *
     * @tparam OtherSampleType type of the sample values in the audio buffer
     * @param buffer audio buffer
     * @param channel index of the channel to use (only mono analysis)
     * @return growth analysis data
    */
    template <typename OtherSampleType>
    GrowthData store (AudioBuffer<OtherSampleType>& buffer, short channel)
    {
        static_assert (std::is_same<OtherSampleType, SampleType>::value,
                       "The sample-type of the Bark module must match the sample-type supplied to this store callback");

        short numChannels = buffer.getNumChannels();
        jassert(channel < numChannels);
        jassert(channel >= 0);
        jassert(numChannels > 0);

        if(channel < 0 || channel >= numChannels)
            throw std::invalid_argument("Channel index has to be between 0 and " + std::to_string(numChannels));
        return perform(buffer.getReadPointer(channel), buffer.getNumSamples());
    }
    /*--------------------------- Setters/getters ----------------------------*/

    /**
     * Set the window function used
     * Sets the window function (options in tIDLib header file)
     * @param func window fuction used
    */
    void setWindowFunction(tIDLib::WindowFunctionType func)
    {
        this->windowFunction = func;
    }

    /**
     * Set the growth thresholds
     * Specify lower and upper growth thresholds. An onset is reported when
     * growth exceeds the upper threshold, then falls below the lower threshold.
     * If lower threshold is set to -1, onsets are reported at the first sign
     * of decay after growth exceeds the upper threshold.
     * The Bark::measure method can help to find appropriate thresholds.
     * @param lo low growth threshold
     * (From wbrent tIDLib bark~.pd help example)
     * @param hi high growth threshold
    */
    void setThresh(float lo, float hi)
    {
        if(hi<lo)
            throw std::invalid_argument("Low threshold is greater than High threshold");

        this->hiThresh = hi;
        this->loThresh = lo;
        this->loThresh = (this->loThresh < 0) ? -1 : this->loThresh;
    }

    std::pair<float,float> getThresh() const
    {
        return std::make_pair(this->loThresh,this->hiThresh );
    }

    /**
     * Set the minimum triggering
     * Set the velocity/amplitude threshold for the detector, which will ignore
     * onsets that are below it. Units are not dB or MIDI velocity, but the sum
     * of the energy in all filterbands.You'll have to fiddle with it based on
     * your input.
     * (From wbrent tIDLib bark~.pd help example)
     *
     * @param mv minimum velocity
    */
    void setMinvel(float mv)
    {
        if(mv < 0)
            throw std::invalid_argument("Minimum triggering velocity must be zero or positive");
        this->minvel = mv;
    }

    /**
     * Set the state of the filter
     * Set whether the triangular Bark spaced filters are used (filterEnabled)
     * OR energy is averaged in the in the unfiltered bins(filterDisabled).
     * (With filterEnabled, sum OR avg are performed depending on which
     * operation is specified with the Bark::setFilterOperation method)
     * (From wbrent tIDLib bark~.pd help example)
     * @param state filter state to set
    */
    void setFiltering(tIDLib::FilterState state)
    {
        this->filterState = state;
    }

    /**
     * Set the operation to perform on the energy in each filter
     * If using the triangular Bark spaced filters, you can either sum or
     * average the energy in each filter .(default: sum)
     * (IF FILTER WAS ENABLED with the Bark::setFiltering method)
     * (From wbrent tIDLib bark~.pd help example)
     * @param operationType type of operation selected
    */
    void setFilterOperation(tIDLib::FilterOperation operationType)
    {
       this->filterOperation = operationType;
    }

    /**
     * Set the Filter range
     * Specify a range of filters to use in the total growth measurement. The
     * appropriate limits for these values depend on how many filters are in the
     * Bark filterbank. With the default spacing of 0.5 Barks, you get 50
     * filters, so setFilterRange(0,49) would instruct bark to use the entire
     * filterbank. If you think you can capture attacks by looking for spectral
     * growth in the high frequencies only, you might want to try
     * setFilterRange(30,49), for instance.
     * (From wbrent tIDLib bark~.pd help example)
     * @param lo lower bin limit
     * @param hi higher bin limit
    */
    void setFilterRange(float lo, float hi)
    {
        if(hi<lo)
            throw std::invalid_argument("Low bin is greater than High bin");
        if(lo < 0)
            throw std::invalid_argument("Low bin is < 0 (must be zero or positive)");
        if(hi >= this->numFilters)
            throw std::invalid_argument("High bin must be smaller than the number of filters ("+std::to_string(this->numFilters)+")");

        this->loBin = lo;
        this->hiBin = hi;
    }

    /**
     * Set mask parameters
     * Specify a number of analysis periods and decay rate for the energy mask.
     * (From wbrent tIDLib bark~.pd help example)
     * @param analysisPeriods The analysis period
     * @param decayRate The decay rate [ range (0.05,0.95) ]
    */
    void setMask(unsigned int analysisPeriods, float decayRate)
    {
        if((decayRate < 0.05) || (decayRate > 0.95))
            throw std::invalid_argument("Mask decay rate must be betweeen 0.05 and 0.95");
        this->maskPeriods = analysisPeriods;
        this->maskDecay = decayRate;
    }

    /**
     * Set debounce time
     * Block onset reports for a given number of millseconds after an onset is
     * detected. The spectral flux that goes on during the first few
     * milliseconds of an instrument's attack can cause extra onset reports.
     * After an onset report, Bark will then suppress further onset reporting
     * until that number the number of debounce milliseconds goes by. This
     * is useful for eliminating a burst of attack reports when the first one
     * is all you really needed.
     * (From wbrent tIDLib bark~.pd help example)
     * @param millis debounce duration in milliseconds
    */
    void setDebounce(int millis)
    {
        if(millis<0)
            throw std::invalid_argument("Debounce time must be >= 0.");
        this->debounceTime = millis;
    }

    /**
     * Get the Debounce Time
     * @return debounce time in millis
    */
    int getDebounceTime() const noexcept
    {
        return this->debounceTime;
    }

    /**
     * Set Debug mode state
     * With debug on (setDebugMode(true)), growth values will be posted for
     * every onset report, giving the peak and lower values.
     * (From wbrent tIDLib bark~.pd help example)
     * @param debug Debug mode state
    */
    void setDebugMode(bool debug)
    {
        this->debug = debug;
        if(debug)
        {
            debugLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(DEBUGLOG_SUBFOLDER, DEBUGLOG_FILENAME, ".txt", "TimbreID bark module - Debug log"));
            this->debugLogger->logMessage("Starting DEBUG log at: " + getDebugLogPath());
        }
    }

    /**
     * Get the Debug Mode state
     * @return the debug mode state
    */
    bool getDebugMode() const noexcept
    {
        return this->isDebugMode;
    }

    /**
     * Set Spew mode state
     * With spew mode on, the list of growth per band and total growth is
     * output on every analysis period. Note that this is different from bonk's
     * spew mode, which provides the power in each band, not the growth.
     * (From wbrent tIDLib bark~.pd help example)
     * @param debug Debug mode state
    */
    void setSpewMode(bool spew)
    {
        this->isSpewMode = spew;
    }

    /**
     * Get the Spew Mode state
     * @return the spew mode state
    */
    bool getSpewMode() const noexcept
    {
        return this->isSpewMode;
    }

    /**
     * Perform a growth measurement
     * Turn measure on, then off after a few seconds to measure average growth.
     * If you measure during an example of relative slience for your input
     * signal, you can get an idea of appropriate growth thresholds, and make
     * changes with the Bark::setThresh method. Peak growth is also reported.
     * (From wbrent tIDLib bark~.pd help example)
     * @param measure measure state
    */
    void measure(bool measure)
    {
        if(measure)
        {
            measureLogger = std::unique_ptr<FileLogger>(FileLogger::createDateStampedLogger(MEASURELOG_SUBFOLDER, MEASURELOG_SUBFOLDER, ".txt", "TimbreID bark module - MEASURE log"));
            this->measureLogger->logMessage("Starting log at: " + getDebugLogPath());
            this->measureLogger->logMessage("Measuring average growth...");
            this->measureTicks = 0;
        }
        else if(measureLogger != nullptr)
        {
            this->measureLogger->logMessage("Number of ticks: "+std::to_string(this->measureTicks));
            this->measureLogger->logMessage("Average growth: "+std::to_string(this->avgGrowth/this->measureTicks));
            this->measureLogger->logMessage("Peak growth: "+std::to_string(this->peakGrowth));
            this->avgGrowth = 0.0;
            this->peakGrowth = 0.0;
            this->measureTicks = UINT_MAX;
        }
    }

    /**
     * Set whether loudness weights are used
     * @param useWeights use weights state
    */
    void setUseWeights(bool useWeights)
    {
        this->useWeights = useWeights;
    }

    /**
     * Get the useWeights state
     * @return the useWeight flag value
    */
    bool getUseWeights() const noexcept
    {
        return this->useWeights;
    }

    /**
     * Get filter frequencies info in std::string from
     * @return a string with filter frequencies
    */
    std::string getFilterFreqsInfo() const noexcept
    {
        std::string res;
        for(t_filterIdx i=0; i<this->numFilters+2; ++i)
            res += "FilterFreq["+std::to_string(i)+"]: "+std::to_string(this->filterFreqs[i]);
        return res;
    }

    /**
     * Set the spectrum type used
     * Set whether the power spectrum or the magnitude spectrum is used
     * @param spec spectrum type
    */
    void setSpectrumType(tIDLib::SpectrumType spec)
    {
        this->spectrumTypeUsed = spec;
        createLoudnessWeighting();
    }

    /**
     * Set whether to normalize the spectrum or not
     * @param normalize normalize flag
    */
    void setNormalize(bool normalize)
    {
        this->normalize = normalize;
    }

    /**
     * Get the analysis window size (in samples)
     * @return analysis window size
    */
    unsigned long int getWindowSize() const noexcept
    {
        return this->analysisWindowSize;
    }

    /**
     * Get the number of samples that are analysed at every period
     * @return hop interval (in samples)
    */
    unsigned long int getHop() const noexcept
    {
        return this->hop;
    }

    /**
     * Get the filter spacing (in Barks)
     * @return filter spacing
    */
    float getBarkSpacing() const noexcept
    {
        return this->barkSpacing;
    }

    /**
     * Get the path to the Debug log folder
     * @return debug log folder path
    */
    String getDebugLogPath() const
    {
        return this->debugLogger->getSystemLogFileFolder().getFullPathName() + "/" + DEBUGLOG_SUBFOLDER + "/";
    }


    /**
     * Get the path to the Measure log folder
     * @return measure log folder path
    */
    String getMeasureLogPath() const
    {
        return this->measureLogger->getSystemLogFileFolder().getFullPathName() + "/" + MEASURELOG_SUBFOLDER + "/";
    }

    /*-----------------------------*/

    /**
        Used to receive callbacks when an onset is detected.

        @see Bark::addListener, Bark::removeListener
    */
    class Listener
    {
    public:
        /** Destructor. */
        virtual ~Listener() = default;

        /** Called when the onset is detected. */
        virtual void onsetDetected (Bark *) = 0;
    };

    /** Registers a listener to receive events when onsets are detected.
        If the listener is already registered, this will not register it again.
        @see removeListener
    */
    void addListener (Listener* newListener)
    {
        barkListeners.add(newListener);
    }

    /** Removes a previously-registered bark listener
        @see addListener
    */
    void removeListener (Listener* listener)
    {
        barkListeners.remove(listener);
    }

private:

    ListenerList<Listener> barkListeners;
    bool isDebugMode = false; // debug MODE
    bool isSpewMode = false;  // spew mode

    unsigned long int sampleRate = tIDLib::SAMPLERATEDEFAULT;   // SampleRate
    unsigned int blockSize = tIDLib::BLOCKSIZEDEFAULT;    // block size
    unsigned long int analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;    // analysisWindowSize

    unsigned long int dspTick = 0;
    unsigned long int hop = tIDLib::WINDOWSIZEDEFAULT*0.25;

    bool normalize = false;
    tIDLib::SpectrumType spectrumTypeUsed = tIDLib::SpectrumType::powerSpectrum;
    bool useWeights = false;
    tIDLib::WindowFunctionType windowFunction = tIDLib::WindowFunctionType::blackman;

    float barkSpacing = tIDLib::BARKSPACINGDEFAULT;
    t_filterIdx numFilters = 0;
    t_filterIdx sizeFilterFreqs;
    std::vector<float> filterFreqs;
    std::vector<tIDLib::t_filter> filterbank;
    tIDLib::FilterState filterState = tIDLib::FilterState::filterEnabled;
    tIDLib::FilterOperation filterOperation = tIDLib::FilterOperation::sumFilterEnergy;        //triangular filter operation type (sum or avg)
    std::vector<float> loudWeights;

    unsigned int measureTicks = UINT_MAX;
    float peakGrowth = 0.0;
    float avgGrowth = 0.0;
    float prevTotalGrowth = 0.0;
    float loThresh = 3;
    float hiThresh = 7;
    float minvel = 1.0;
    /* band range: loBin through hiBin (inclusive) */
    t_filterIdx loBin;
    t_filterIdx hiBin;

    int debounceTime = 100;
    float maskDecay = 0.7;
    unsigned int maskPeriods = 4;
    std::vector<t_filterIdx> numPeriods; // t_filterIdx type because this buffer is used to check making per filter band

    bool debounceActive = false;    //system flag, stays true for debounceTime millis after every hit
    bool haveHit = false;

    std::vector<float> blackman;
    std::vector<float> cosine;
    std::vector<float> hamming;
    std::vector<float> hann;

    std::vector<SampleType> signalBuffer;

    std::vector<float> fftwInputVector;
    float *fftwIn;
    fftwf_complex *fftwOut;
    fftwf_plan fftwPlan;

    std::vector<float> mask;
    std::vector<float> growth;

    std::unique_ptr<FileLogger> debugLogger;
    std::unique_ptr<FileLogger> measureLogger;

    /* Utility functions -----------------------------------------------------*/

    void createLoudnessWeighting()
    {
        std::vector<float> barkFreqs(this->numFilters,0.0f);

        float barkSum = this->barkSpacing;

        for(t_filterIdx i = 0; i < this->numFilters; ++i)
        {
            barkFreqs[i] = tIDLib::bark2freq(barkSum);
            barkSum += this->barkSpacing;
        }

        for(t_filterIdx i = 0; i < this->numFilters; ++i)
        {
            t_binIdx nearIdx = tIDLib::nearestBinIndex(barkFreqs[i], weights_freqs, tIDLib::NUMWEIGHTPOINTS);
            float nearFreq = weights_freqs[nearIdx];
            float diffdB = 0.0;

            float diffFreq, dBint;

            // wbrent original comment:
            // this doesn't have to be if/else'd into a greater/less situation.  later on i should write a more general interpolation solution, and maybe move it up to 4 points instead.
            if(barkFreqs[i]>nearFreq)
            {
                if(nearIdx <= tIDLib::NUMWEIGHTPOINTS-2)
                {
                    diffFreq = (barkFreqs[i] - nearFreq)/(weights_freqs[nearIdx+1] - nearFreq);
                    diffdB = diffFreq * (weights_dB[nearIdx+1] - weights_dB[nearIdx]);
                }

                dBint = weights_dB[nearIdx] + diffdB;
            }
            else
            {
                if(nearIdx > 0)
                {
                    diffFreq = (barkFreqs[i] - weights_freqs[nearIdx-1])/(nearFreq - weights_freqs[nearIdx-1]);
                    diffdB = diffFreq * (weights_dB[nearIdx] - weights_dB[nearIdx-1]);
                }

                dBint = weights_dB[nearIdx-1] + diffdB;
            }

            switch(this->spectrumTypeUsed)
            {
                case tIDLib::SpectrumType::powerSpectrum:
                    this->loudWeights[i] = powf(10.0, dBint*0.1);
                    break;
                case tIDLib::SpectrumType::magnitudeSpectrum:
                    this->loudWeights[i] = powf(10.0, dBint*0.05);
                    break;
                default:
                    throw std::logic_error("Spectrum type option not available!");
            }

        }
    }

    /* END Utility functions -------------------------------------------------*/

    void timerCallback() override
    {
        Timer::stopTimer();
        clearHit();
    }

    void resizeBuffer()
    {
        signalBuffer.resize(this->analysisWindowSize + this->blockSize,SampleType{0});
    }

    void clearHit()
    {
        this->debounceActive = false;
    }

    void initModule()
    {
        resizeBuffer();
        reset();

        this->blackman.resize(this->analysisWindowSize);
        this->cosine.resize(this->analysisWindowSize);
        this->hamming.resize(this->analysisWindowSize);
        this->hann.resize(this->analysisWindowSize);
         // initialize signal windowing functions
        tIDLib::initBlackmanWindow(this->blackman);
        tIDLib::initCosineWindow(this->cosine);
        tIDLib::initHammingWindow(this->hamming);
        tIDLib::initHannWindow(this->hann);

        this->fftwInputVector.resize(this->analysisWindowSize);
        this->fftwIn = &this->fftwInputVector[0];

        // set up the FFTW output buffer
        this->fftwOut = (fftwf_complex *)fftwf_alloc_complex(this->analysisWindowSize*0.5 + 1);
        fftwf_alloc_complex(this->analysisWindowSize);
        // DFT plan
        this->fftwPlan = fftwf_plan_dft_r2c_1d(this->analysisWindowSize, this->fftwIn, this->fftwOut, FFTWPLANNERFLAG);

        // we're supposed to initialize the input array after we create the plan
        for(unsigned long int i=0; i<this->analysisWindowSize; ++i)
            this->fftwIn[i] = 0.0;

        sizeFilterFreqs = tIDLib::getBarkBoundFreqs(this->filterFreqs, this->barkSpacing, this->sampleRate);

        jassert(sizeFilterFreqs == this->filterFreqs.size());

        // sizeFilterFreqs-2 is the correct number of filters, since we don't count the start point of the first filter, or the finish point of the last filter
        this->numFilters = this->sizeFilterFreqs-2;

        tIDLib::createFilterbank(this->filterFreqs, this->filterbank, this->numFilters, this->analysisWindowSize, this->sampleRate);

        this->loBin = 0;
        this->hiBin = this->numFilters-1;

        this->mask.resize(this->numFilters, 0.0f);
        this->growth.resize(this->numFilters, 0.0f);
        this->numPeriods.resize(this->numFilters, 0);
        this->loudWeights.resize(this->numFilters, 0.0f);

        createLoudnessWeighting();
    }

    void outputOnset()
    {
        barkListeners.call([this] (Listener& l) { l.onsetDetected (this); });
    }

    GrowthData perform(const SampleType* input, size_t n)
    {
        jassert(n ==  this->blockSize);

        GrowthData growthData;
        unsigned long int window, windowHalf;
        float totalGrowth, totalVel;
        std::vector<float> *windowFuncPtr;

        window = this->analysisWindowSize;
        windowHalf = this->analysisWindowSize*0.5;

         // shift signal buffer contents back.
        for(unsigned long int i=0; i<window; ++i)
            this->signalBuffer[i] = this->signalBuffer[i+n];

        // write new block to end of signal buffer.
        for(unsigned long int i=0; i<n; ++i)
            this->signalBuffer[window+i] = input[i];

        this->dspTick += n;

        if(this->dspTick >= this->hop)
        {
            this->dspTick = 0;
            totalGrowth = 0.0;
            totalVel = 0.0;

            switch(this->windowFunction)
            {
                case tIDLib::WindowFunctionType::rectangular:
                    break;
                case tIDLib::WindowFunctionType::blackman:
                    windowFuncPtr = &this->blackman;
                    break;
                case tIDLib::WindowFunctionType::cosine:
                    windowFuncPtr = &this->cosine;
                    break;
                case tIDLib::WindowFunctionType::hamming:
                    windowFuncPtr = &this->hamming;
                    break;
                case tIDLib::WindowFunctionType::hann:
                    windowFuncPtr = &this->hann;
                    break;
                default:
                    windowFuncPtr = &this->blackman;
                    break;
            };

            // if windowFunction == 0, skip the windowing (rectangular)
            if(this->windowFunction != tIDLib::WindowFunctionType::rectangular)
            {
                for(unsigned long int i=0; i<window; ++i)
                    this->fftwIn[i] = this->signalBuffer[i] * windowFuncPtr->at(i);
            }
            else
            {
                for(unsigned long int i=0; i<window; ++i)
                    this->fftwIn[i] = this->signalBuffer[i];
            }

            fftwf_execute(this->fftwPlan);

            // put the result of power calc back in fftwIn
            tIDLib::power(windowHalf+1, this->fftwOut, this->fftwIn);

            if(this->spectrumTypeUsed == tIDLib::SpectrumType::magnitudeSpectrum)
                tIDLib::mag(windowHalf+1, this->fftwIn);

            switch(this->filterState)
            {
                case tIDLib::FilterState::filterDisabled:
                    tIDLib::specFilterBands(windowHalf+1, this->numFilters, this->fftwIn, this->filterbank, this->normalize);
                    break;
                case tIDLib::FilterState::filterEnabled:
                    tIDLib::filterbankMultiply(this->fftwIn, this->normalize, this->filterOperation, this->filterbank, this->numFilters);
                    break;
                default:
                    throw std::logic_error("Filter option not available");
                    break;
            }

             // optional loudness weighting
            if(this->useWeights)
                for(unsigned long int i=0; i<this->numFilters; ++i)
                    this->fftwIn[i] *= this->loudWeights[i];

            for(unsigned long int i=0; i < this->numFilters; ++i)
            {
                totalVel += this->fftwIn[i];

                // init growth list to zero
                this->growth[i] = 0.0;

                // from p.3 of Puckette/Apel/Zicarelli, 1998
                // salt divisor with + 1.0e-15 in case previous power was zero
                if(this->fftwIn[i] > this->mask[i])
                    this->growth[i] = this->fftwIn[i]/(this->mask[i] + 1.0e-15) - 1.0;

                if(i>=this->loBin && i<=this->hiBin && this->growth[i]>0)
                    totalGrowth += this->growth[i];
            }

            if(this->measureTicks != UINT_MAX)
            {
                if(totalGrowth > this->peakGrowth)
                    this->peakGrowth = totalGrowth;

                this->avgGrowth += totalGrowth;
                this->measureTicks++;
            }

            if(totalVel >= this->minvel && totalGrowth > this->hiThresh && !this->haveHit && !this->debounceActive)
            {
                 if(this->isDebugMode)
                     this->debugLogger->logMessage("Peak: " + std::to_string(totalGrowth));

                this->haveHit = true;
                this->debounceActive = true;
                Timer::startTimer(this->debounceTime); // wait debounceTime ms before allowing another attack
            }
            else if(this->haveHit && this->loThresh>0 && totalGrowth < this->loThresh) // if loThresh is an actual value (not -1), then wait until growth drops below that value before reporting attack
            {
                if(this->isDebugMode)
                    this->debugLogger->logMessage("Drop: "+std::to_string(totalGrowth));

                this->haveHit = false;

                // don't output data if spew will do it anyway below
                if(!this->isSpewMode)
                    growthData.setData(growth,totalGrowth);

                outputOnset();
            }
            else if(this->haveHit && this->loThresh<0 && totalGrowth < this->prevTotalGrowth) // if loThresh == -1, report attack as soon as growth shows any decay at all
            {
                if(this->isDebugMode)
                    this->debugLogger->logMessage("Drop: "+std::to_string(totalGrowth));

                this->haveHit = false;

                // don't output data if spew will do it anyway below
                if(!this->isSpewMode)
                    growthData.setData(growth,totalGrowth);

                outputOnset();
            }

            if(this->isSpewMode)
                growthData.setData(growth,totalGrowth);

            // update mask
            for(unsigned long int i=0; i<this->numFilters; ++i)
            {
                if(this->fftwIn[i] > this->mask[i])
                {
                    this->mask[i] = this->fftwIn[i];
                    this->numPeriods[i] = 0;
                }
                else
                {
                    this->numPeriods[i]++;
                    if(this->numPeriods[i] >= this->maskPeriods)
                        this->mask[i] *= this->maskDecay;
                }
            }

            this->prevTotalGrowth = totalGrowth;
        }

        return growthData;
    }

    void freeMem()
    {
        // free FFTW stuff
        fftwf_free(this->fftwOut);
        fftwf_destroy_plan(this->fftwPlan);
    }
};

} // namespace tid
