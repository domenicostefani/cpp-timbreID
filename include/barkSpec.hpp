/*

barkSpec - header between Juce and the TimbreID barkSpec~ module
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 20th April 2020
  (domenico.stefani96@gmail.com)

Porting from barkSpec~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/barkSpec~.c/

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#pragma once

#include "tIDLib.hpp"
#include "fftw3.h"
#include <stdexcept>
#include <cassert>

namespace tid   /* TimbreID namespace*/
{

template <typename SampleType>
class BarkSpec
{
public:

    /** Creates a BarkSpec module with default parameters. */
    BarkSpec()
    {
        this->analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;
        this->barkSpacing = tIDLib::BARKSPACINGDEFAULT;
        initModule();
    }

    /**
     * BarkSpec constructor
     * Creates a BarkSpec module specifying an additional parameter
     * @param analysisWindowSize size of the analysis window
    */
    BarkSpec(unsigned long int analysisWindowSize)
    {
        if (analysisWindowSize < tIDLib::MINWINDOWSIZE)
            throw std::invalid_argument("Window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater.");

        this->analysisWindowSize = analysisWindowSize;
        this->barkSpacing = tIDLib::BARKSPACINGDEFAULT;

        initModule();
    }

    /**
     * BarkSpec constructor
     * Creates a BarkSpec module specifying additional parameters
     * @param analysisWindowSize size of the analysis window
     * @param barkSpacing filter spacing in Barks
    */
    BarkSpec(unsigned long int analysisWindowSize, float barkSpacing)
    {
        if (analysisWindowSize < tIDLib::MINWINDOWSIZE)
            throw std::invalid_argument("Window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater.");
        if (barkSpacing < tIDLib::MINBARKSPACING || barkSpacing > tIDLib::MAXBARKSPACING)
            throw std::invalid_argument("Bark spacing must be between "+std::to_string(tIDLib::MINBARKSPACING)+" and "+std::to_string(tIDLib::MAXBARKSPACING)+" Barks");

        this->analysisWindowSize = analysisWindowSize;
        this->barkSpacing = barkSpacing;
        initModule();
    }

    /** Creates a copy of another BarkSpec module. */
    BarkSpec (const BarkSpec&) = default;

    /** Move constructor */
    BarkSpec (BarkSpec&&) = default;

    /** Free memory used */
    ~BarkSpec() { freeMem(); }

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
        this->sampleRate = sampleRate;
        if (blockSize != this->blockSize)
        {
            this->blockSize = blockSize;
            this->signalBuffer.resize(this->analysisWindowSize + this->blockSize);
        }
        reset();
    }

    /**
     * Resets the processing pipeline.
     * It empties the analysis buffer
    */
    void reset() noexcept
    {
        std::fill(this->signalBuffer.begin(), this->signalBuffer.end(), SampleType{0});
    }

    /**
     * Construct a new filterbank with a specific spacing.
     * @param barkSpacing spacing for the new filterbank (in Barks)
    */
    void createFilterbank(float barkSpacing)
    {
        if (barkSpacing < tIDLib::MINBARKSPACING || barkSpacing > tIDLib::MAXBARKSPACING)
            throw std::invalid_argument("Bark spacing must be between "+std::to_string(tIDLib::MINBARKSPACING)+" and "+std::to_string(tIDLib::MAXBARKSPACING)+" Barks");

        this->barkSpacing = barkSpacing;

        this->sizeFilterFreqs = tIDLib::getBarkBoundFreqs(this->filterFreqs, this->barkSpacing, this->sampleRate);
        assert(sizeFilterFreqs == this->filterFreqs.size());

        // sizeFilterFreqs-2 is the correct number of filters, since we don't count the start point of the first filter, or the finish point of the last filter
        this->numFilters = this->sizeFilterFreqs-2;

        tIDLib::createFilterbank(this->filterFreqs, this->filterbank, this->numFilters, this->analysisWindowSize, this->sampleRate);

        // resize listOut memory
        this->listOut.resize(this->numFilters);
    }

    /**
     * Stores an audio block into the buffer of the module
     * It stores the audio block provided into a buffer where analysis is
     * performed.
     *
     * @tparam OtherSampleType type of the sample values in the audio buffer
     * @param buffer audio buffer
     * @param channel index of the channel to use (only mono analysis)
    */
    template <typename OtherSampleType>
    void store (AudioBuffer<OtherSampleType>& buffer, short channel)
    {
        static_assert (std::is_same<OtherSampleType, SampleType>::value,
                       "The sample-type of the module must match the sample-type supplied to this store callback");

        short numChannels = buffer.getNumChannels();

        if(channel < 0 || channel >= numChannels)
            throw std::invalid_argument("Channel index has to be between 0 and "+std::to_string(numChannels-1)+" (found "+std::to_string(channel)+" instead)");
        storeAudioBlock(buffer.getReadPointer(channel), buffer.getNumSamples());
    }

    /**
     * Compute the bark spec coefficients
     * @return bark spec coefficients
    */
    std::vector<float>& compute()
    {
        std::vector<float> *windowFuncPtr;

        unsigned long int windowHalf = this->analysisWindowSize * 0.5f;

       #if ASYNC_FEATURE_EXTRACTION
        uint32_t currentTime = tid::Time::getTimeSince(this->lastStoreTime);
        uint32_t offsetSample = roundf((currentTime / 1000.0f) * this->sampleRate);
        if (offsetSample >= this->blockSize)
            offsetSample = this->blockSize-1;
       #else
        if ((tIDLib::FEATURE_EXTRACTION_OFFSET < 0.0f) || (tIDLib::FEATURE_EXTRACTION_OFFSET > 1.0f)) throw new std::logic_error("FEATURE_EXTRACTION_OFFSET must be between 0.0 and 1.0 (found "+std::to_string(tIDLib::FEATURE_EXTRACTION_OFFSET)+" instead)");
        uint32_t offsetSample = (unsigned long int)(tIDLib::FEATURE_EXTRACTION_OFFSET * (double)this->blockSize);
       #endif

        // construct analysis window using offsetSample as the end of the window
        for (unsigned long int i=0, j=offsetSample; i<this->analysisWindowSize; ++i, ++j)
            this->fftwInputVector[i] = this->signalBuffer[j];

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

        // if windowFunction == rectangular, skip the windowing (rectangular)
        if (this->windowFunction != tIDLib::WindowFunctionType::rectangular)
            for (unsigned long int i=0; i<this->analysisWindowSize; ++i)
                this->fftwInputVector[i] *= windowFuncPtr->at(i);

        fftwf_execute(this->fftwPlan);

        // put the result of power calc back in fftwIn
        float* fftwIn = &(fftwInputVector[0]);
        tIDLib::power(windowHalf+1, this->fftwOut, fftwIn);

        if (this->spectrumTypeUsed != tIDLib::SpectrumType::powerSpectrum)
            tIDLib::mag(windowHalf+1, fftwIn);

        switch(this->filterState)
        {
            case tIDLib::FilterState::filterDisabled: // like the old x_specBandAvg == true
                tIDLib::specFilterBands(windowHalf+1, this->numFilters, fftwIn, this->filterbank, this->normalize);
                break;
            case tIDLib::FilterState::filterEnabled:
                tIDLib::filterbankMultiply(fftwIn, this->normalize, this->filterOperation, this->filterbank, this->numFilters);
                break;
            default:
                throw std::logic_error("Filter option not available");
                break;
        }

        for (t_filterIdx i = 0; i < this->numFilters; ++i)
            this->listOut[i] = this->fftwInputVector[i];

        return this->listOut;
    }

    /*--------------------------- Setters/getters ----------------------------*/

    /**
     * Set the window function used
     * Sets the window function (options in tIDLib header file)
     * @param func window fuction used
    */
    void setWindowFunction(tIDLib::WindowFunctionType func) noexcept
    {
        this->windowFunction = func;
    }

    /**
     * Set the state of the filter
     * Set whether the triangular Bark spaced filters are used (filterEnabled)
     * OR energy is averaged in the in the unfiltered bins(filterDisabled).
     * (With filterEnabled, sum OR avg are performed depending on which
     * operation is specified with the Bark::setFilterOperation method)
     * (From wbrent help examples)
     * @param state filter state to set
    */
    void setFiltering(tIDLib::FilterState state) noexcept
    {
        this->filterState = state;
    }

    /**
     * Set the operation to perform on the energy in each filter
     * If using the triangular Bark spaced filters, you can either sum or
     * average the energy in each filter .(default: sum)
     * (IF FILTER WAS ENABLED with the setFiltering method)
     * (From wbrent help examples)
     * @param operationType type of operation selected
    */
    void setFilterOperation(tIDLib::FilterOperation operationType) noexcept
    {
       this->filterOperation = operationType;
    }

    /**
     * Set the spectrum type used
     * Set whether the power spectrum or the magnitude spectrum is used
     * @param spec spectrum type
    */
    void setSpectrumType(tIDLib::SpectrumType spec) noexcept
    {
        this->spectrumTypeUsed = spec;
    }

    /**
     * Set analysis window size
     * Set a window size value for the analysis. Note that this can be also done
     * from the constructor if there is no need for a change in this value.
     * Window size is not required to be a power of two.
     * @param windowSize the size of the window
    */
    void setWindowSize(uint32_t windowSize)
    {
        // FFT must be at least 4 points long
        if (windowSize < 4)
            throw std::invalid_argument("Window size must be 4 or greater");
        this->analysisWindowSize = windowSize;

        unsigned long int windowHalf = windowSize * 0.5f;

        this->signalBuffer.resize(this->analysisWindowSize + this->blockSize);
        this->fftwInputVector.resize(this->analysisWindowSize);

        // free the FFTW output buffer, and re-malloc according to new window
        fftwf_free(this->fftwOut);

        // destroy old DFT plan, which depended on this->analysisWindowSize
        fftwf_destroy_plan(this->fftwPlan);

        // allocate new fftwf_complex memory for the plan based on new window size
        this->fftwOut = (fftwf_complex *) fftwf_alloc_complex(windowHalf+1);

        // create a new DFT plan based on new window size
        float* fftwIn = &(fftwInputVector[0]);
        this->fftwPlan = fftwf_plan_dft_r2c_1d(this->analysisWindowSize, fftwIn, this->fftwOut, FFTWPLANNERFLAG);

        // we're supposed to initialize the input array after we create the plan
         for (unsigned long int i=0; i<this->analysisWindowSize; ++i)
            this->fftwInputVector[i] = 0.0f;

        // initialize signal buffer
        for (unsigned long int i=0; i<this->analysisWindowSize+this->blockSize; ++i)
            this->signalBuffer[i] = 0.0f;

        this->blackman.resize(this->analysisWindowSize);
        this->cosine.resize(this->analysisWindowSize);
        this->hamming.resize(this->analysisWindowSize);
        this->hann.resize(this->analysisWindowSize);

        // initialize signal windowing functions
        tIDLib::initBlackmanWindow(this->blackman);
        tIDLib::initCosineWindow(this->cosine);
        tIDLib::initHammingWindow(this->hamming);
        tIDLib::initHannWindow(this->hann);

        tIDLib::createFilterbank(this->filterFreqs, this->filterbank, this->numFilters, this->analysisWindowSize, this->sampleRate);
    }

    /**
     *  Set the Spectrum Normalization mode (on/off)
     * @param norm normalization flag value
    */
    void setNormalize(bool norm) noexcept
    {
        this->normalize = norm;
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
     * Get the normalize flag value
     * @return analysis window size
    */
    bool getNormalize() const noexcept
    {
        return this->normalize;
    }

    /**
     * Get filter frequencies info in std::string from
     * @return a string with filter frequencies
    */

    std::string getFilterFreqsInfo() const noexcept
    {
        std::string res = "";

        for (t_filterIdx i = 0; i < this->numFilters + 2; ++i)
        {
            res += "filterFreqs[";
            res += std::to_string(i);
            res += "]:";
            res += std::to_string(this->filterFreqs[i]);
        }
        return res;
    }

    /**
     * Return a string containing the main parameters of the module.
     * Refer to the PD helper files of the original timbreID library to know more:
     * https://github.com/wbrent/timbreID/tree/master/help
     * @return string with parameter info
    */
    std::string getInfoString() const
    {
        std::string res = "";

        res += "Samplerate: ";
        res += std::to_string((unsigned long int)(this->sampleRate));

        res += "\nBlock size: ";
        res += std::to_string(this->blockSize);

        res += "\nWindow: ";
        res += std::to_string(this->analysisWindowSize);

        res += "\nNormalize: ";
        res += std::to_string(this->normalize);

        res += "\npower spectrum: ";
        res += this->spectrumTypeUsed == tIDLib::SpectrumType::powerSpectrum ? "true (using power spectrum)" : "false (using magnitude spectrum)";

        res += "\nwindow function: ";
        switch(this->windowFunction)
        {
            case tIDLib::WindowFunctionType::rectangular:
                res += "rectangular (no windowing)";
                break;
            case tIDLib::WindowFunctionType::blackman:
                res += "blackman";
                break;
            case tIDLib::WindowFunctionType::cosine:
                res += "cosine";
                break;
            case tIDLib::WindowFunctionType::hamming:
                res += "hamming";
                break;
            case tIDLib::WindowFunctionType::hann:
                res += "hann";
                break;
            default:
                throw std::logic_error("Invalid window function");
                break;
        };

        res += "\nBark spacing: ";
        res += std::to_string(this->barkSpacing);

        res += "\nNumber of filters: ";
        res += std::to_string(this->numFilters);

        res += "\nspectrum band averaging: ";
        res += this->filterState == tIDLib::FilterState::filterDisabled ? "true (filterDisabled)" : "false (filterEnabled)";

        res += "\ntriangular filter averaging: ";
        res += this->filterOperation == tIDLib::FilterOperation::averageFilterEnergy ? "true (FilterOperation = avg)" : "false (FilterOperation = sum)";

        return res;
    }

    /**
     * Return a string containing information abount a specific filter
     * @param filt filter index
     * @return info string
    */
    std::string getHatString(float filt) const noexcept
    {
        t_filterIdx idx;
        std::string res = "";

        idx = filt;

        if (idx >= this->numFilters)
        {
            res = "filter " + std::to_string(idx) + "does not exist.";
        }
        else
        {
            res += "filter size[";
            res += std::to_string(idx);
            res += "]: ";
            res += std::to_string(this->filterbank[idx].size);

            for (t_binIdx i=0; i < this->filterbank[idx].size; ++i)
            {
                res += "\nval ";
                res += std::to_string(i);
                res += ": ";
                res += std::to_string(this->filterbank[idx].filter[i]);
            }

            res += "\nidxLo: ";
            res += std::to_string(this->filterbank[idx].indices[0]);
            res += ", idxHi: ";
            res += std::to_string(this->filterbank[idx].indices[1]);
        }

        return res;
    }

private:

    /**
     * Initialize the parameters of the module.
    */
    void initModule()
    {
        this->sampleRate = tIDLib::SAMPLERATEDEFAULT;
        this->blockSize = tIDLib::BLOCKSIZEDEFAULT;
        this->windowFunction = tIDLib::WindowFunctionType::blackman;
        this->normalize = true;
        this->spectrumTypeUsed = tIDLib::SpectrumType::magnitudeSpectrum;
       #if ASYNC_FEATURE_EXTRACTION
        this->lastStoreTime = tid::Time::currentTimeMillis();
       #endif
        this->sizeFilterFreqs = 0;
        this->numFilters = 0; // this is just an init size that will be updated in createFilterbank anyway.
        this->filterState = tIDLib::FilterState::filterEnabled;
        this->filterOperation = tIDLib::FilterOperation::sumFilterEnergy;

        this->signalBuffer.resize(this->analysisWindowSize + this->blockSize);
        this->fftwInputVector.resize(this->analysisWindowSize);

        // initialize signal buffer
        for (unsigned long int i = 0; i < this->analysisWindowSize + this->blockSize; ++i)
            this->signalBuffer[i] = 0.0f;

        this->blackman.resize(this->analysisWindowSize);
        this->cosine.resize(this->analysisWindowSize);
        this->hamming.resize(this->analysisWindowSize);
        this->hann.resize(this->analysisWindowSize);

        // initialize signal windowing functions
        tIDLib::initBlackmanWindow(this->blackman);
        tIDLib::initCosineWindow(this->cosine);
        tIDLib::initHammingWindow(this->hamming);
        tIDLib::initHannWindow(this->hann);

        // set up the FFTW output buffer
        this->fftwOut = (fftwf_complex *)fftwf_alloc_complex(this->analysisWindowSize * 0.5f + 1);

        // DFT plan
        float* fftwIn = &(fftwInputVector[0]);
        this->fftwPlan = fftwf_plan_dft_r2c_1d(this->analysisWindowSize, fftwIn, this->fftwOut, FFTWPLANNERFLAG);

        // we're supposed to initialize the input array after we create the plan
         for (unsigned long int i = 0; i < this->analysisWindowSize; ++i)
            this->fftwInputVector[i] = 0.0f;

        this->sizeFilterFreqs = tIDLib::getBarkBoundFreqs(this->filterFreqs, this->barkSpacing, this->sampleRate);
        assert(this->sizeFilterFreqs == this->filterFreqs.size());


        // sizeFilterFreqs-2 is the correct number of filters, since we don't count the start point of the first filter, or the finish point of the last filter
        this->numFilters = this->sizeFilterFreqs-2;

        tIDLib::createFilterbank(this->filterFreqs, this->filterbank, this->numFilters, this->analysisWindowSize, this->sampleRate);

        // create listOut memory
        this->listOut.resize(this->numFilters);
    }

    /**
     * Buffer the content of the input audio block
     * @param input audio block to store
     * @param n size of the block
    */
    void storeAudioBlock(const SampleType* input, size_t n) noexcept
    {
        // shift signal buffer contents back.
        for (unsigned long int i=0; i < this->analysisWindowSize; ++i)
            this->signalBuffer[i] = this->signalBuffer[i+n];

        // write new block to end of signal buffer.
        for (unsigned long int i=0; i < n; ++i)
            this->signalBuffer[this->analysisWindowSize+i] = input[i];

       #if ASYNC_FEATURE_EXTRACTION
        this->lastStoreTime = tid::Time::currentTimeMillis();
       #endif
    }

    /**
     *  Free memory allocated
    */
    void freeMem()
    {
        fftwf_free(this->fftwOut);
        fftwf_destroy_plan(this->fftwPlan);
    }

    //==========================================================================

    float sampleRate;
    float blockSize;

    unsigned long int analysisWindowSize;

    tIDLib::WindowFunctionType windowFunction;
    tIDLib::SpectrumType spectrumTypeUsed;   // replaces x_powerSpectrum

   #if ASYNC_FEATURE_EXTRACTION
    uint32_t lastStoreTime; // replaces x_lastDspTime
   #endif

    std::vector<SampleType> signalBuffer;

    std::vector<float> fftwInputVector;
    fftwf_complex *fftwOut;
    fftwf_plan fftwPlan;

    std::vector<float> blackman;
    std::vector<float> cosine;
    std::vector<float> hamming;
    std::vector<float> hann;

    t_filterIdx sizeFilterFreqs;
    t_filterIdx numFilters;

    float barkSpacing;
    std::vector<float> filterFreqs;
    std::vector<tIDLib::t_filter> filterbank;

    tIDLib::FilterState filterState;         // replaces x_specBandAvg
    tIDLib::FilterOperation filterOperation; // replaces x_filterAvg
    bool normalize;

    std::vector<float> listOut;
};

} // namespace tid
