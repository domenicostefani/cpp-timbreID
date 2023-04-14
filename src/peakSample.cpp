/*

template <typename SampleType>
PeakSample<SampleType>::PeakSample - Bridge-header between Juce and the TimbreID peakSample method
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

#include "peakSample.h"

namespace tid   /* TimbreID namespace*/
{

//==============================================================================
/** Creates a PeakSample module with default parameters. */

template <typename SampleType>
PeakSample<SampleType>::PeakSample(){ resizeBuffers(); reset(); }

template <typename SampleType>
PeakSample<SampleType>::PeakSample(unsigned long int windowSize){
    this->analysisWindowSize = windowSize;
    resizeBuffers();
    reset();
}


//==============================================================================
/** Initialization of the module */
template <typename SampleType>
void PeakSample<SampleType>::prepare (double sampleRate, uint32_t blockSize) noexcept
{
    if((sampleRate != this->sampleRate) || (blockSize != this->blockSize))
    {
        this->sampleRate = sampleRate;
        this->blockSize = blockSize;
        resizeBuffers();
    }
    reset();
}

/** Resets the processing pipeline. */
template <typename SampleType>
void PeakSample<SampleType>::reset() noexcept
{
    std::fill(signalBuffer.begin(), signalBuffer.end(), SampleType{0});
    std::fill(analysisBuffer.begin(), analysisBuffer.end(), 0.0f);
    #if ASYNC_FEATURE_EXTRACTION
    this->lastStoreTime = tid::Time::currentTimeMillis();
    #endif
}

//==============================================================================

template <typename SampleType>
void PeakSample<SampleType>::compute(float &_peak, unsigned long int &_peakIdx)
{
    #if ASYNC_FEATURE_EXTRACTION
    uint32_t currentTime = tid::Time::getTimeSince(this->lastStoreTime);
    if(currentTime > blockSize*sampleRate)
        throw std::logic_error("Clock measure may have overflowed");

    uint32_t offsetSample = roundf((currentTime / 1000.0) * this->sampleRate);

    // If the period was too long, we cap the maximum number of samples, which is @blockSize
    if(offsetSample >= this->blockSize)
        offsetSample = this->blockSize - 1;
    #else
    if ((tIDLib::FEATURE_EXTRACTION_OFFSET < 0.0f) || (tIDLib::FEATURE_EXTRACTION_OFFSET > 1.0f)) throw new std::logic_error("FEATURE_EXTRACTION_OFFSET must be between 0.0 and 1.0 (found "+std::to_string(tIDLib::FEATURE_EXTRACTION_OFFSET)+" instead)");
    uint32_t offsetSample = (uint32_t)(tIDLib::FEATURE_EXTRACTION_OFFSET * (double)this->blockSize);
    #endif

    // construct analysis window using offsetSample as the end of the window
    for(uint64_t i = 0, j = offsetSample; i < this->analysisWindowSize; ++i, ++j)
        this->analysisBuffer[i] = (float)this->signalBuffer[j];

    assert(this->analysisBuffer.size() == this->analysisWindowSize);

    _peak = -FLT_MAX;
    _peakIdx = ULONG_MAX;

    for(size_t i = 0; i < analysisWindowSize; ++i)
    {
        float thisSample = fabs(this->analysisBuffer[i]);
        if(thisSample > _peak)
        {
            _peak = thisSample;
            _peakIdx = i;
        }
    }
}

/**
 * \deprecated
 * Kept for compatibility, probably bad in RT context
*/
template <typename SampleType>
std::pair<float, unsigned long int> PeakSample<SampleType>::compute()
{
    float peak;
    unsigned long int peakIdx;
    this->compute(peak,peakIdx);
    return std::make_pair(peak,peakIdx);
}

template <typename SampleType>
void PeakSample<SampleType>::setWindowSize(uint32_t windowSize)
{
    this->analysisWindowSize = windowSize;
    resizeBuffers();
}

template <typename SampleType>
uint32_t PeakSample<SampleType>::getWindowSize() const
{
    return this->analysisWindowSize;
}

/**
 * Return a string containing the main parameters of the module.
 * Refer to the PD helper files of the original timbreID library to know more:
 * https://github.com/wbrent/timbreID/tree/master/help
 * @return string with parameter info
*/
template <typename SampleType>
std::string PeakSample<SampleType>::getInfoString() const noexcept
{
    std::string res = "";

    res += "Samplerate: ";
    res += std::to_string((unsigned long int)(this->sampleRate));

    res += "\nBlock size: ";
    res += std::to_string(this->blockSize);

    res += "\nWindow: ";
    res += std::to_string(this->analysisWindowSize);

    return res;
}

template <typename SampleType>
void PeakSample<SampleType>::resizeBuffers()
{
    signalBuffer.resize(analysisWindowSize + blockSize,SampleType{0});
    analysisBuffer.resize(analysisWindowSize,0.0f);
}

template <typename SampleType>
void PeakSample<SampleType>::storeAudioBlock (const SampleType* input, size_t n) noexcept
{
    assert(n ==  this->blockSize);

    // shift signal buffer contents N positions back
    for(uint64_t i=0; i<analysisWindowSize; ++i)
        signalBuffer[i] = signalBuffer[i+n];

    // write new block to end of signal buffer.
    for(size_t i=0; i<n; ++i)
        signalBuffer[analysisWindowSize+i] = input[i];

    #if ASYNC_FEATURE_EXTRACTION
    this->lastStoreTime = tid::Time::currentTimeMillis();
    #endif
}

template class PeakSample<float>;
template class PeakSample<double>;

} // namespace tid
