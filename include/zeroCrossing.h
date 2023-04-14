/*

zeroCrossing - Bridge-header between Juce and the TimbreID zeroCrossing method
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 24th March 2020
  (domenico.stefani96@gmail.com)

Porting from zeroCrossing~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/zeroCrossing~.c/

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

namespace tid   /* TimbreID namespace*/
{

template <typename SampleType>
class ZeroCrossing
{
public:

    //==============================================================================
    /** Creates a zeroCrossing module with default parameters. */
    ZeroCrossing();

    ZeroCrossing(unsigned long int windowSize);

    /** Creates a copy of another zeroCrossing module. */
    ZeroCrossing (const ZeroCrossing&) = default;

    /** Move constructor */
    ZeroCrossing (ZeroCrossing&&) = default;

    //==============================================================================
    /** Initialization of the module */
    void prepare (double sampleRate, uint32_t blockSize) noexcept;

    /** Resets the processing pipeline. */
    void reset() noexcept;

    void storeAudioBlock (const SampleType* input, size_t n) noexcept;
    //==============================================================================

    uint32_t compute();

    void setWindowSize(uint32_t windowSize);

    uint32_t getWindowSize() const;

    /**
     * Return a string containing the main parameters of the module.
     * Refer to the PD helper files of the original timbreID library to know more:
     * https://github.com/wbrent/timbreID/tree/master/help
     * @return string with parameter info
    */
    std::string getInfoString() const noexcept;

private:

    void resizeBuffers();

    //==============================================================================
    double sampleRate = tIDLib::SAMPLERATEDEFAULT;  // x_sr field in Original PD library
    uint32_t blockSize = tIDLib::BLOCKSIZEDEFAULT;    // x_n field in Original PD library library
    uint64_t analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;   // x_window in Original PD library

    std::vector<SampleType> signalBuffer;
    std::vector<float> analysisBuffer;

   #if ASYNC_FEATURE_EXTRACTION
    uint32_t lastStoreTime = tid::Time::currentTimeMillis(); // x_lastDspTime in Original PD library
   #endif
};

} // namespace tid
