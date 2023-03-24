/*

peakSample - Bridge-header between Juce and the TimbreID peakSample method
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 26th March 2020
  (domenico.stefani96@gmail.com)

Porting from peakSample~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/peakSample~.c/

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#pragma once

namespace tid   /* TimbreID namespace*/
{

template <typename SampleType>
class PeakSample
{
public:

    //==============================================================================
    /** Creates a PeakSample module with default parameters. */
    PeakSample();

    PeakSample(unsigned long int windowSize);

    /** Creates a copy of another PeakSample module. */
    PeakSample (const PeakSample&) = default;

    /** Move constructor */
    PeakSample (PeakSample&&) = default;

    //==============================================================================
    /** Initialization of the module */
    void prepare (double sampleRate, uint32_t blockSize) noexcept;

    /** Resets the processing pipeline. */
    void reset() noexcept;

    void storeAudioBlock (const SampleType* input, size_t n) noexcept;
    //==============================================================================

    void compute(float &_peak, unsigned long int &_peakIdx);

    /**
     * \deprecated
     * Kept for compatibility, probably bad in RT context
     * TODO: check
    */
    std::pair<float, unsigned long int> compute();
    
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
