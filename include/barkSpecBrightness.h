/*

barkSpecBrightness - header between Juce and the TimbreID barkSpecBrightness~ module
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 14th April 2020
  (domenico.stefani96@gmail.com)

Porting from barkSpecBrightness~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/barkSpecBrightness~.c/

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#pragma once

#include "tIDLib.h"
#include "fftw3.h"
#include <stdexcept>
#include <cassert>

#define DEFAULTBOUNDARY 8.5 //TODO: handle this

namespace tid
{

template <typename SampleType>
class BarkSpecBrightness
{
public:
    /** Creates a BarkSpecBrightness module with default parameters. */
    BarkSpecBrightness();

    /**
     * BarkSpecBrightness constructor
     * Creates a BarkSpecBrightness module specifying an additional parameter
     * @param analysisWindowSize size of the analysis window
    */
    BarkSpecBrightness(unsigned long int analysisWindowSize);

    /**
     * BarkSpecBrightness constructor
     * Creates a BarkSpecBrightness module specifying additional parameters
     * @param analysisWindowSize size of the analysis window
     * @param barkSpacing filter spacing in Barks
    */
    BarkSpecBrightness(unsigned long int analysisWindowSize, float barkSpacing);

    /**
     * BarkSpecBrightness constructor
     * Creates a BarkSpecBrightness module specifying additional parameters
     * @param analysisWindowSize size of the analysis window
     * @param barkSpacing filter spacing in Barks
     * @param barkBoundary boundary point (in Barks)
    */
    BarkSpecBrightness(unsigned long int analysisWindowSize, float barkSpacing, float barkBoundary);

    /** Creates a copy of another BarkSpecBrightness module. */
    BarkSpecBrightness (const BarkSpecBrightness&) = default;

    /** Move constructor */
    BarkSpecBrightness (BarkSpecBrightness&&) = default;

    /** Free memory used */
    ~BarkSpecBrightness();

    //==========================================================================
    /**
     * Initialization of the module
     * Prepares the module to play by clearing buffers and setting audio buffers
     * standard parameters
     * @param sampleRate The sample rate of the buffer
     * @param blockSize The size of the individual audio blocks
    */
    void prepare (unsigned long int sampleRate, unsigned int blockSize) noexcept;

    /**
     * Resets the processing pipeline.
     * It empties the analysis buffer
    */
    void reset() noexcept;

    /**
     * Construct a new filterbank with a specific spacing.
     * @param barkSpacing spacing for the new filterbank (in Barks)
    */
    void createFilterbank(float barkSpacing);

    /**
     * Buffer the content of the input audio block
     * @param input audio block to store
     * @param n size of the block
    */
    void storeAudioBlock(const SampleType* input, size_t n) noexcept;

    /**
     * Compute the brightness value
     * @return brightness value
    */
    float compute();

    /*--------------------------- Setters/getters ----------------------------*/

    /**
     * Set the window function used
     * Sets the window function (options in tIDLib header file)
     * @param func window fuction used
    */
    void setWindowFunction(tIDLib::WindowFunctionType func);

    /**
     * Set the spectrum type used
     * Set whether the power spectrum or the magnitude spectrum is used
     * @param spec spectrum type
    */
    void setSpectrumType(tIDLib::SpectrumType spec);

    /**
     * Set the state of the filter
     * Set whether the triangular Bark spaced filters are used (filterEnabled)
     * OR energy is averaged in the in the unfiltered bins(filterDisabled).
     * (With filterEnabled, sum OR avg are performed depending on which
     * operation is specified with the Bark::setFilterOperation method)
     * (From wbrent tIDLib bark~.pd help example)
     * @param state filter state to set
    */
    void setFiltering(tIDLib::FilterState state);

    /**
     * Set the operation to perform on the energy in each filter
     * If using the triangular Bark spaced filters, you can either sum or
     * average the energy in each filter .(default: sum)
     * (IF FILTER WAS ENABLED with the BarkSpecBrightness::setFiltering method)
     * (From wbrent tIDLib bark~.pd help example)
     * @param operationType type of operation selected
    */
    void setFilterOperation(tIDLib::FilterOperation operationType);
    /**
     * Set analysis window size
     * Set a window size value for the analysis. Note that this can be also done
     * from the constructor if there is no need for a change in this value.
     * Window size is not required to be a power of two.
     * @param windowSize the size of the window
    */
    void setWindowSize(uint32_t windowSize);

    /**
     * Get the analysis window size (in samples)
     * @return analysis window size
    */
    unsigned long int getWindowSize() const noexcept;

    /**
     * Return a string containing the main parameters of the module.
     * Refer to the PD helper files of the barkSpecBrightness~ and bark~ modules
     * of the original timbreID library to know more:
     * https://github.com/wbrent/timbreID/tree/master/help
     * @return string with parameter info
    */
    std::string getInfoString();

    /**
     * Set boundary point in Barks
     * @param barkBoundary boundary point
    */
    void setBoundary(float barkBoundary);

    /**
     * Get boundary point in Barks
     * @return boundary point
    */
    float getBoundary();

private:

    /**
     * Initialize the parameters of the module.
    */
    void initModule();
    /**
     * free the memory used by fftw3
    */
    void freeMem();

    //==========================================================================

    float sampleRate;
    float blockSize;

    unsigned long int analysisWindowSize;

    tIDLib::WindowFunctionType windowFunction;
    tIDLib::SpectrumType spectrumTypeUsed;

   #if ASYNC_FEATURE_EXTRACTION
    uint32_t lastStoreTime; // lastDspTime in Original PD library
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

    std::vector<float> barkFreqList;
    float barkSpacing;
    std::vector<float> filterFreqs;
    std::vector<tIDLib::t_filter> filterbank;

    tIDLib::FilterState filterState;
    tIDLib::FilterOperation filterOperation;
    float barkBoundary;
    float freqBoundary;
    t_filterIdx bandBoundary;
};

}    /* TimbreID namespace*/