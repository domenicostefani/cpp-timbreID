/*

Cepstrum - header between Juce and the TimbreID cepstrum~ module
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 21th April 2020
  (domenico.stefani96@gmail.com)

Porting from cepstrum~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/cepstrum~.c/

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

namespace tid   /* TimbreID namespace*/
{

template <typename SampleType>
class Cepstrum
{
public:

    /** Creates a Cepstrum module with default parameters. */
    Cepstrum()
    {
        this->analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;
        initModule();
    }

    /**
     * Cepstrum constructor
     * Creates a Cepstrum module specifying an additional parameter
     * @param analysisWindowSize size of the analysis window
    */
    Cepstrum(unsigned long int analysisWindowSize)
    {
        if (analysisWindowSize < tIDLib::MINWINDOWSIZE)
            throw std::invalid_argument("Window size must be " + std::to_string(tIDLib::MINWINDOWSIZE) + " or greater.");

        this->analysisWindowSize = analysisWindowSize;
        initModule();
    }

    /** Creates a copy of another Cepstrum module. */
    Cepstrum (const Cepstrum&) = default;

    /** Move constructor */
    Cepstrum (Cepstrum&&) = default;

    /** Free memory used */
    ~Cepstrum() { freeMem(); }

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

    /*--------------------------- Setters/getters ----------------------------*/

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
        jassert(channel < numChannels);
        jassert(channel >= 0);
        jassert(numChannels > 0);

        if(channel < 0 || channel >= numChannels)
            throw std::invalid_argument("Channel index has to be between 0 and " + std::to_string(numChannels));
        return storeAudioBlock(buffer.getReadPointer(channel), buffer.getNumSamples());
    }

    /**
     * Compute the cepstrum coefficients
     * @return cepstrum coefficients
    */
    std::vector<float> compute()
    {
        std::vector<float> *windowFuncPtr;
        unsigned long int windowHalf = this->analysisWindowSize * 0.5;

        uint32 currentTime = tid::Time::getTimeSince(this->lastStoreTime);
        uint32 bangSample = roundf((currentTime / 1000.0) * this->sampleRate);

        if (bangSample >= this->blockSize)
            bangSample = this->blockSize - 1;

        // construct analysis window using bangSample as the end of the window
        for (unsigned long int i = 0, j = bangSample; i < this->analysisWindowSize; ++i, ++j)
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

        // if windowFunction == 0, skip the windowing (rectangular)
        if (this->windowFunction != tIDLib::WindowFunctionType::rectangular)
            for (unsigned long int i=0; i<this->analysisWindowSize; ++i)
                this->fftwInputVector[i] *= windowFuncPtr->at(i);

        fftwf_execute(this->fftwForwardPlan);

        // put the result of power calc back in fftwIn
        float* fftwIn = &(this->fftwInputVector[0]);
        tIDLib::power(windowHalf + 1, this->fftwOut, fftwIn);

        if (this->spectrumTypeUsed != tIDLib::SpectrumType::powerSpectrum)
            tIDLib::mag(windowHalf + 1, fftwIn);

        // add 1.0 to power or magnitude spectrum before taking the log and then IFT. Avoid large negative values from log(negativeNum)
        if (this->spectrumOffset)
            for (unsigned long int i = 0; i < windowHalf + 1; ++i)
                this->fftwInputVector[i] += 1.0;

        tIDLib::veclog(windowHalf + 1, fftwIn);   // this can also be called on a std::vector

        // copy forward DFT magnitude result into real part of backward DFT complex input buffer, and zero out the imaginary part. fftwOut is only N/2 + 1 points long, while fftwIn is N points long
        for (unsigned long int i=0; i<windowHalf + 1; ++i)
        {
            this->fftwOut[i][0] = this->fftwInputVector[i];
            this->fftwOut[i][1] = 0.0;
        }

        fftwf_execute(this->fftwBackwardPlan);

        for (unsigned long int i = 0; i < windowHalf + 1; ++i)
            this->fftwInputVector[i] *= (1.0f / this->analysisWindowSize);

        // optionally square the cepstrum results for power cepstrum
        if (this->cepstrumTypeUsed == tIDLib::CepstrumType::powerCepstrum)
            for (unsigned long int i = 0; i < windowHalf + 1; ++i)
                this->fftwInputVector[i] = this->fftwInputVector[i] * this->fftwInputVector[i];

        for (unsigned long int i = 0; i < windowHalf + 1; ++i)
            this->listOut[i] = this->fftwInputVector[i];

         return(this->listOut);
    }

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
     * Set the spectrum type used
     * Set whether the power spectrum or the magnitude spectrum is used
     * @param spec spectrum type
    */
    void setSpectrumType(tIDLib::SpectrumType spec) noexcept
    {
        this->spectrumTypeUsed = spec;
    }

    /**
     * Set the cepstrum type reported
     * Set whether the power cepstrum or the magnitude cepstrum is reported
     * @param ceps cepstrum type
    */
    void setCepstrumType(tIDLib::CepstrumType ceps) noexcept
    {
        this->cepstrumTypeUsed = ceps;
    }

    /**
     * Set the SpectrumOffset state
     * Set wehther the spectrum offset is ON or OFF
     * @param offset SpectrumOffset state
    */
    void setSpectrumOffset(bool offset)
    {
        this->spectrumOffset = offset;
    }

    /**
     * Set analysis window size
     * Set a window size value for the analysis. Note that this can be also done
     * from the constructor if there is no need for a change in this value.
     * Window size is not required to be a power of two.
     * @param windowSize the size of the window
    */
    void setWindowSize(uint32 windowSize)
    {
        if (windowSize < tIDLib::MINWINDOWSIZE)
            throw std::invalid_argument("Window size must be " + std::to_string(tIDLib::MINWINDOWSIZE) + " or greater");
        this->analysisWindowSize = windowSize;

        unsigned long int windowHalf = this->analysisWindowSize * 0.5;

        this->signalBuffer.resize(this->analysisWindowSize + this->blockSize);
        this->fftwInputVector.resize(this->analysisWindowSize);
        this->listOut.resize(windowHalf + 1);

        // free the FFTW output buffer, and re-malloc according to new window
        fftwf_free(this->fftwOut);

        // destroy old plan, which depended on this->analysisWindowSize
        fftwf_destroy_plan(this->fftwForwardPlan);
        fftwf_destroy_plan(this->fftwBackwardPlan);

        // allocate new fftwf_complex memory for the plan based on new window size
        this->fftwOut = (fftwf_complex *) fftwf_alloc_complex(windowHalf + 1);

        // create a new DFT plan based on new window size
        float* fftwIn = &(this->fftwInputVector[0]);
        this->fftwForwardPlan = fftwf_plan_dft_r2c_1d(this->analysisWindowSize, fftwIn, this->fftwOut, FFTWPLANNERFLAG);
        this->fftwBackwardPlan = fftwf_plan_dft_c2r_1d(this->analysisWindowSize, this->fftwOut, fftwIn, FFTWPLANNERFLAG);

        // we're supposed to initialize the input array after we create the plan
        for (unsigned long int i = 0; i < this->analysisWindowSize; ++i)
            this->fftwInputVector[i] = 0.0;

        // initialize signal buffer
        for (unsigned long int i = 0; i < this->analysisWindowSize + this->blockSize; ++i)
            this->signalBuffer[i] = 0.0;

        this->blackman.resize(this->analysisWindowSize);
        this->cosine.resize(this->analysisWindowSize);
        this->hamming.resize(this->analysisWindowSize);
        this->hann.resize(this->analysisWindowSize);

        // initialize signal windowing functions
        tIDLib::initBlackmanWindow(this->blackman);
        tIDLib::initCosineWindow(this->cosine);
        tIDLib::initHammingWindow(this->hamming);
        tIDLib::initHannWindow(this->hann);
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
     * Return a string containing the main parameters of the module.
     * Refer to the PD helper files of the original timbreID library to know more:
     * https://github.com/wbrent/timbreID/tree/master/help
     * @return string with parameter info
    */
    std::string getInfoString() const noexcept
    {
        std::string res = "";

        res += "Samplerate: ";
        res += std::to_string((unsigned long int)(this->sampleRate));

        res += "\nBlock size: ";
        res += std::to_string(this->blockSize);

        res += "\nWindow: ";
        res += std::to_string(this->analysisWindowSize);

        res += "\nPower spectrum: ";
        res += this->spectrumTypeUsed == tIDLib::SpectrumType::powerSpectrum ? "true (powerSpectrum)" : "false (magnitudeSpectrum)";

        res += "\nPower cepstrum: ";
        res += this->cepstrumTypeUsed == tIDLib::CepstrumType::powerCepstrum ? "true (powerCepstrum)" : "false (magnitudeCepstrum)";

        res += "\nSpectrum offset: ";
        res += std::to_string(this->spectrumOffset);

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
                res += "ERROR! Invalid window function";
                break;
        };

        return res;
    }

private:

    /**
     * Buffer the content of the input audio block
     * @param input audio block to store
     * @param n size of the block
    */
    void storeAudioBlock(const SampleType* input, size_t n) noexcept
    {
        // shift signal buffer contents back.
        for (unsigned long int i = 0; i < this->analysisWindowSize; ++i)
            this->signalBuffer[i] = this->signalBuffer[i + n];

        // write new block to end of signal buffer.
        for (unsigned long int i = 0; i < n; ++i)
            this->signalBuffer[this->analysisWindowSize + i] = input[i];

        this->lastStoreTime = juce::Time::currentTimeMillis();
    }

    /**
     * Initialize the parameters of the module.
    */
    void initModule()
    {
        this->sampleRate = tIDLib::SAMPLERATEDEFAULT;
        this->blockSize = tIDLib::BLOCKSIZEDEFAULT;
        this->windowFunction = tIDLib::WindowFunctionType::blackman;
        this->spectrumTypeUsed = tIDLib::SpectrumType::magnitudeSpectrum;
        this->cepstrumTypeUsed = tIDLib::CepstrumType::magnitudeCepstrum;
        this->lastStoreTime = juce::Time::currentTimeMillis();
        this->spectrumOffset = false;

        this->signalBuffer.resize(this->analysisWindowSize + this->blockSize);
        this->fftwInputVector.resize(this->analysisWindowSize);
        this->listOut.resize(this->analysisWindowSize * 0.5 + 1);

        for (unsigned long int i = 0; i < (this->analysisWindowSize + this->blockSize); ++i)
            this->signalBuffer[i] = 0.0;

        this->blackman.resize(this->analysisWindowSize);
        this->cosine.resize(this->analysisWindowSize);
        this->hamming.resize(this->analysisWindowSize);
        this->hann.resize(this->analysisWindowSize);

        // initialize signal windowing functions
        tIDLib::initBlackmanWindow(this->blackman);
        tIDLib::initCosineWindow(this->cosine);
        tIDLib::initHammingWindow(this->hamming);
        tIDLib::initHannWindow(this->hann);

        // set up the FFTW output buffer.
        this->fftwOut = (fftwf_complex *)fftwf_alloc_complex(this->analysisWindowSize * 0.5 + 1);

        // Forward DFT plan
        float* fftwIn = &(this->fftwInputVector[0]);
        this->fftwForwardPlan = fftwf_plan_dft_r2c_1d(this->analysisWindowSize, fftwIn, this->fftwOut, FFTWPLANNERFLAG);

        // Backward DFT plan
        this->fftwBackwardPlan = fftwf_plan_dft_c2r_1d(this->analysisWindowSize, this->fftwOut, fftwIn, FFTWPLANNERFLAG);

        // we're supposed to initialize the input array after we create the plan
        for (unsigned long int i = 0; i < this->analysisWindowSize; ++i)
            this->fftwInputVector[i] = 0.0;
    }

    /**
     *  Free memory allocated
    */
    void freeMem()
    {
        // free FFTW stuff
        fftwf_free(this->fftwOut);
        fftwf_destroy_plan(this->fftwForwardPlan);
        fftwf_destroy_plan(this->fftwBackwardPlan);
    }

    //==========================================================================

    float sampleRate;
    float blockSize;

    unsigned long int analysisWindowSize;

    tIDLib::WindowFunctionType windowFunction;
    tIDLib::SpectrumType spectrumTypeUsed;   // replaces x_powerSpectrum
    tIDLib::CepstrumType cepstrumTypeUsed;   // replaces x_powerCepstrum
    bool spectrumOffset;

    uint32 lastStoreTime; // replaces x_lastDspTime

    std::vector<SampleType> signalBuffer;

    std::vector<float> fftwInputVector;
    fftwf_complex *fftwOut;
    fftwf_plan fftwForwardPlan;
    fftwf_plan fftwBackwardPlan;

    std::vector<float> blackman;
    std::vector<float> cosine;
    std::vector<float> hamming;
    std::vector<float> hann;

    std::vector<float> listOut;
};

} // namespace tid
