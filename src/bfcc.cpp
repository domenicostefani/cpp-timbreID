/*

bfcc - Bark Frequency Cepstral Coefficients
     > header between Juce and the TimbreID bfcc~ module
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 20th April 2020
  (domenico.stefani96@gmail.com)

Porting from bfcc~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/bfcc~.c/

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "bfcc.h"

namespace tid   /* TimbreID namespace*/
{


/** Creates a Bfcc module with default parameters. */
template <typename SampleType>
Bfcc<SampleType>::Bfcc()
{
    this->analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;
    this->barkSpacing = tIDLib::BARKSPACINGDEFAULT;
    initModule();
}

/**
 * Bfcc constructor
 * Creates a Bfcc module specifying an additional parameter
 * @param analysisWindowSize size of the analysis window
*/
template <typename SampleType>
Bfcc<SampleType>::Bfcc(unsigned long int analysisWindowSize)
{
    if (analysisWindowSize < tIDLib::MINWINDOWSIZE)
        throw std::invalid_argument("Window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater.");

    this->analysisWindowSize = analysisWindowSize;
    this->barkSpacing = tIDLib::BARKSPACINGDEFAULT;

    initModule();
}

/**
 * Bfcc constructor
 * Creates a Bfcc module specifying additional parameters
 * @param analysisWindowSize size of the analysis window
 * @param barkSpacing filter spacing in Barks
*/
template <typename SampleType>
Bfcc<SampleType>::Bfcc(unsigned long int analysisWindowSize, float barkSpacing)
{
    if (analysisWindowSize < tIDLib::MINWINDOWSIZE)
        throw std::invalid_argument("Window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater.");
    if (barkSpacing < tIDLib::MINBARKSPACING || barkSpacing > tIDLib::MAXBARKSPACING)
        throw std::invalid_argument("Bark spacing must be between "+std::to_string(tIDLib::MINBARKSPACING)+" and "+std::to_string(tIDLib::MAXBARKSPACING)+" Barks");

    this->analysisWindowSize = analysisWindowSize;
    this->barkSpacing = barkSpacing;
    initModule();
}

/** Free memory used */
template <typename SampleType>
Bfcc<SampleType>::~Bfcc() { freeMem(); }

//==========================================================================

/**
 * Initialization of the module
 * Prepares the module to play by clearing buffers and setting audio buffers
 * standard parameters
 * @param sampleRate The sample rate of the buffer
 * @param blockSize The size of the individual audio blocks
*/
template <typename SampleType>
void Bfcc<SampleType>::prepare (unsigned long int sampleRate, unsigned int blockSize) noexcept
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
template <typename SampleType>
void Bfcc<SampleType>::reset() noexcept
{
    std::fill(this->signalBuffer.begin(), this->signalBuffer.end(), SampleType{0});
}

/**
 * Construct a new filterbank with a specific spacing.
 * @param barkSpacing spacing for the new filterbank (in Barks)
*/
template <typename SampleType>
void Bfcc<SampleType>::createFilterbank(float barkSpacing)
{
    if (barkSpacing < tIDLib::MINBARKSPACING || barkSpacing > tIDLib::MAXBARKSPACING)
        throw std::invalid_argument("Bark spacing must be between "+std::to_string(tIDLib::MINBARKSPACING)+" and "+std::to_string(tIDLib::MAXBARKSPACING)+" Barks");

    this->barkSpacing = barkSpacing;

    this->sizeFilterFreqs = tIDLib::getBarkBoundFreqs(this->filterFreqs, this->barkSpacing, this->sampleRate);
    assert(sizeFilterFreqs == this->filterFreqs.size());

    // sizeFilterFreqs-2 is the correct number of filters, since we don't count the start point of the first filter, or the finish point of the last filter
    this->numFilters = this->sizeFilterFreqs-2;

    tIDLib::createFilterbank(this->filterFreqs, this->filterbank, this->numFilters, this->analysisWindowSize, this->sampleRate);

    this->coefficientsVector.resize(this->numFilters);
    this->dctPlan.precomputeBasis(this->numFilters);
}

/**
 * Compute the bark frequency cepstral coefficients
 * @return cepstral coefficients
*/
template <typename SampleType>
std::vector<float>& Bfcc<SampleType>::compute()
{
    std::vector<float> *windowFuncPtr;

    unsigned long int windowHalf = this->analysisWindowSize * 0.5f;

    #if ASYNC_FEATURE_EXTRACTION
    uint32_t currentTime = tid::Time::getTimeSince(this->lastStoreTime);
    uint32_t offsetSample = roundf((currentTime/1000.0f)*this->sampleRate);
    if (offsetSample >= this->blockSize)
        offsetSample = this->blockSize-1;
    #else
    if ((tIDLib::FEATURE_EXTRACTION_OFFSET < 0.0f) || (tIDLib::FEATURE_EXTRACTION_OFFSET > 1.0f)) throw new std::logic_error("FEATURE_EXTRACTION_OFFSET must be between 0.0 and 1.0 (found "+std::to_string(tIDLib::FEATURE_EXTRACTION_OFFSET)+" instead)");
    uint32_t offsetSample = (uint32_t)(tIDLib::FEATURE_EXTRACTION_OFFSET * (double)this->blockSize);
    #endif

    // construct analysis window using offsetSample as the end of the window
    for (unsigned long int i = 0, j = offsetSample; i < this->analysisWindowSize; ++i, ++j)
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
    tIDLib::power(windowHalf+1, this->fftwOut, &fftwInputVector[0]);

    if (this->spectrumTypeUsed != tIDLib::SpectrumType::powerSpectrum)
        tIDLib::mag(windowHalf+1, &fftwInputVector[0]);

    switch(this->filterState)
    {
        case tIDLib::FilterState::filterDisabled: // like the old x_specBandAvg == true
            tIDLib::specFilterBands(windowHalf+1, this->numFilters, &fftwInputVector[0], this->filterbank, this->normalize);
            break;
        case tIDLib::FilterState::filterEnabled:
            tIDLib::filterbankMultiply(&fftwInputVector[0], this->normalize, this->filterOperation, this->filterbank, this->numFilters);
            break;
        default:
            throw std::logic_error("Filter option not available");
            break;
    }

    // FFTW DCT-II
    dctPlan.compute(fftwInputVector,coefficientsVector,this->numFilters);

    return this->coefficientsVector;
}

/*--------------------------- Setters/getters ----------------------------*/

/**
 * Set the window function used
 * Sets the window function (options in tIDLib header file)
 * @param func window fuction used
*/
template <typename SampleType>
void Bfcc<SampleType>::setWindowFunction(tIDLib::WindowFunctionType func) noexcept
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
template <typename SampleType>
void Bfcc<SampleType>::setFiltering(tIDLib::FilterState state) noexcept
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
template <typename SampleType>
void Bfcc<SampleType>::setFilterOperation(tIDLib::FilterOperation operationType) noexcept
{
    this->filterOperation = operationType;
}

/**
 * Set the spectrum type used
 * Set whether the power spectrum or the magnitude spectrum is used
 * @param spec spectrum type
*/
template <typename SampleType>
void Bfcc<SampleType>::setSpectrumType(tIDLib::SpectrumType spec) noexcept
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
template <typename SampleType>
void Bfcc<SampleType>::setWindowSize(uint32_t windowSize)
{
    if (windowSize < tIDLib::MINWINDOWSIZE)
        throw std::invalid_argument("Window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater");
    this->analysisWindowSize = windowSize;

    unsigned long int windowHalf = windowSize*0.5f;

    this->signalBuffer.resize(this->analysisWindowSize + this->blockSize);
    this->fftwInputVector.resize(this->analysisWindowSize);

    // free the FFTW output buffer, and re-malloc according to new window
    fftwf_free(this->fftwOut);

    // destroy old DFT plan, which depended on this->analysisWindowSize
    fftwf_destroy_plan(this->fftwPlan);

    // allocate new fftwf_complex memory for the plan based on new window size
    this->fftwOut = (fftwf_complex *) fftwf_alloc_complex(windowHalf + 1);

    // create a new DFT plan based on new window size
    this->fftwPlan = fftwf_plan_dft_r2c_1d(this->analysisWindowSize, &(fftwInputVector[0]), this->fftwOut, FFTWPLANNERFLAG);

    // we're supposed to initialize the input array after we create the plan
    for (unsigned long int i = 0; i < this->analysisWindowSize; ++i)
        this->fftwInputVector[i] = 0.0f;

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

    tIDLib::createFilterbank(this->filterFreqs, this->filterbank, this->numFilters, this->analysisWindowSize, this->sampleRate);
}

/**
 *  Set the normalization flag
 * @param norm normalization flag value
*/
template <typename SampleType>
void Bfcc<SampleType>::setNormalize(bool norm) noexcept
{
    this->normalize = norm;
}

/**
 * Get the analysis window size (in samples)
 * @return analysis window size
*/
template <typename SampleType>
unsigned long int Bfcc<SampleType>::getWindowSize() const noexcept
{
    return this->analysisWindowSize;
}

/**
 * Get the normalize flag value
 * @return analysis window size
*/
template <typename SampleType>
bool Bfcc<SampleType>::getNormalize() const noexcept
{
    return this->normalize;
}

/**
 * Return a string containing the main parameters of the module.
 * Refer to the PD helper files of the original timbreID library to know more:
 * https://github.com/wbrent/timbreID/tree/master/help
 * @return string with parameter info
*/
template <typename SampleType>
std::string Bfcc<SampleType>::getInfoString() const
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

    res += "\nPower spectrum: ";
    res += this->spectrumTypeUsed == tIDLib::SpectrumType::powerSpectrum ? "true (powerSpectrum)" : "false (magnitudeSpectrum)";

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
 * Initialize the parameters of the module.
*/
template <typename SampleType>
void Bfcc<SampleType>::initModule()
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
    this->fftwPlan = fftwf_plan_dft_r2c_1d(this->analysisWindowSize, &(fftwInputVector[0]), this->fftwOut, FFTWPLANNERFLAG);

    // we're supposed to initialize the input array after we create the plan
    for (unsigned long int i=0; i<this->analysisWindowSize; ++i)
        this->fftwInputVector[i] = 0.0f;

    this->sizeFilterFreqs = tIDLib::getBarkBoundFreqs(this->filterFreqs, this->barkSpacing, this->sampleRate);
    assert(this->sizeFilterFreqs == this->filterFreqs.size());

    // sizeFilterFreqs-2 is the correct number of filters, since we don't count the start point of the first filter, or the finish point of the last filter
    this->numFilters = this->sizeFilterFreqs-2;

    tIDLib::createFilterbank(this->filterFreqs, this->filterbank, this->numFilters, this->analysisWindowSize, this->sampleRate);

    this->coefficientsVector.resize(this->numFilters);
    this->dctPlan.precomputeBasis(this->numFilters);
}

/**
 * Buffer the content of the input audio block
 * @param input audio block to store
 * @param n size of the block
*/
template <typename SampleType>
void Bfcc<SampleType>::storeAudioBlock(const SampleType* input, size_t n) noexcept
{
    // shift signal buffer contents back.
    for (unsigned long int i = 0; i < this->analysisWindowSize; ++i)
        this->signalBuffer[i] = this->signalBuffer[i+n];

    // write new block to end of signal buffer.
    for (unsigned long int i = 0; i < n; ++i)
        this->signalBuffer[this->analysisWindowSize+i] = input[i];

    #if ASYNC_FEATURE_EXTRACTION
    this->lastStoreTime = tid::Time::currentTimeMillis();
    #endif
}

/**
 *  Free memory allocated
*/
template <typename SampleType>
void Bfcc<SampleType>::freeMem()
{
    // free FFTW stuff
    fftwf_free(this->fftwOut);
    fftwf_destroy_plan(this->fftwPlan);
}

template class Bfcc<float>;
// template class Bfcc<double>; // Problems with fftwf


} // namespace tid
