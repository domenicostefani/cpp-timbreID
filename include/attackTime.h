/*

attackTime - Bridge-header between Juce and the TimbreID attackTime~ method
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 26th March 2020
  (domenico.stefani96@gmail.com)

Porting from AttackTime~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/attackTime~.c/

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#pragma once

#include "tIDLib.h"
#include <vector>
#include <cassert>
#include <climits>  // ULONG_MAX

namespace tid   /* TimbreID namespace*/
{

template <typename SampleType>
class AttackTime
{
public:

    //==============================================================================
    /** Creates a AttackTime module with default parameters. */
    AttackTime();

    /** Creates a AttackTime module with custom analysis window size. */
    AttackTime(unsigned long int windowSize);

    /** Creates a copy of another AttackTime module. */
    AttackTime (const AttackTime&) = default;

    /** Move constructor */
    AttackTime (AttackTime&&) = default;

    //==============================================================================
    /** Initialization of the module */
    void prepare (double sampleRate, uint32_t blockSize) noexcept;
    /** Resets the processing pipeline. */
    void reset() noexcept;

    void storeAudioBlock (const SampleType* input, size_t n) noexcept;
    //==============================================================================

    void compute(unsigned long int* rPeakSampIdx, unsigned long int* rAttackStartIdx, float *rAttackTime);

    void setWindowSize(uint32_t windowSize);

    uint32_t getWindowSize() const;

    /**
     * Specify the maximum time in milliseconds before the peak sample of an
     * attack to search for the initial onset. This determines the size of the
     * internal buffer and is separate from the analysis window size
     * (default: 2000ms)
    */
    void setMaxSearchRange(float rangeMillis);
    unsigned long int getMaxSearchRange() const;

    void setSampMagThresh(float thresh);

    float getSampMagThresh() const;

    void setNumSampsThresh(unsigned long int thresh);

    unsigned long int getNumSampsThresh() const;

    /**
     * Return a string containing the main parameters of the module.
     * Refer to the PD helper files of the original timbreID library to know more:
     * https://github.com/wbrent/timbreID/tree/master/help
     * @return string with parameter info
    */
    std::string getInfoString() const noexcept;

private:

    void resizeAnalysisBuffer();

    void resizeAllBuffers();

    //==============================================================================
    double sampleRate = tIDLib::SAMPLERATEDEFAULT;  // x_sr field in Original PD library
    uint32_t blockSize = tIDLib::BLOCKSIZEDEFAULT;    // x_n field in Original PD library library
    uint64_t analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;   // x_window in Original PD library

    /** minimum sample threshold for finding onset */
    unsigned short int numSampsThresh= 10;

    /** sample magnitude threshold for finding onset */
	float sampMagThresh = 0.005;

    /** maximum search range */
    unsigned long int maxSearchRange = this->sampleRate * 2.0; // two seconds

    std::vector<SampleType> signalBuffer;
    std::vector<float> analysisBuffer;
    std::vector<float> searchBuffer;

   #if ASYNC_FEATURE_EXTRACTION
    uint32_t lastStoreTime = tid::Time::currentTimeMillis(); // x_lastDspTime in Original PD library
   #endif
};

} // namespace tid
