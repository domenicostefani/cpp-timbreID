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

#include "tIDLib.hpp"
#include <vector>

namespace tid   /* TimbreID namespace*/
{

template <typename SampleType>
class ZeroCrossing
{
public:

    //==============================================================================
    /** Creates a zeroCrossing module with default parameters. */
    ZeroCrossing(){ resizeBuffers(); reset(); }

    ZeroCrossing(unsigned long int windowSize)
    {
        this->analysisWindowSize = windowSize;
        resizeBuffers();
        reset();
    }

    /** Creates a copy of another zeroCrossing module. */
    ZeroCrossing (const ZeroCrossing&) = default;

    /** Move constructor */
    ZeroCrossing (ZeroCrossing&&) = default;

    //==============================================================================
    /** Initialization of the module */
    void prepare (double sampleRate, uint32 blockSize) noexcept
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
    void reset() noexcept
    {
        std::fill(signalBuffer.begin(), signalBuffer.end(), SampleType{0});
        std::fill(analysisBuffer.begin(), analysisBuffer.end(), 0.0f);
       #if ASYNC_FEATURE_EXTRACTION
        this->lastStoreTime = juce::Time::currentTimeMillis();
       #endif
    }

    template <typename OtherSampleType>
    void store (AudioBuffer<OtherSampleType>& buffer, short channel)
    {
        static_assert (std::is_same<OtherSampleType, SampleType>::value,
                       "The sample-type of the zeroCrossing module must match the sample-type supplied to this store callback");

        short numChannels = buffer.getNumChannels();

        jassert(channel < numChannels);
        jassert(channel >= 0);

        if(channel < 0 || channel >= numChannels)
            throw std::logic_error("Channel index has to be between 0 and " + std::to_string(numChannels-1));
        storeAudioBlock(buffer.getReadPointer(channel), buffer.getNumSamples());
    }
    //==============================================================================

    uint32 compute()
    {
       #if ASYNC_FEATURE_EXTRACTION
        uint32 currentTime = tid::Time::getTimeSince(this->lastStoreTime);
        if(currentTime > blockSize*sampleRate)
            throw std::logic_error("Clock measure may have overflowed");
        uint32 bangSample = roundf((currentTime / 1000.0) * this->sampleRate);
        // If the period was too long, we cap the maximum number of samples, which is @blockSize
        if(bangSample >= this->blockSize)
            bangSample = this->blockSize - 1;
       #else
        uint32 bangSample = 0;
       #endif

    	// construct analysis window using bangSample as the end of the window
        for(uint64 i = 0, j = bangSample; i < this->analysisWindowSize; ++i, ++j)
            this->analysisBuffer[i] = (float)this->signalBuffer[j];

        uint32 crossings = 0;

        jassert(this->analysisBuffer.size() == this->analysisWindowSize);
        crossings = tIDLib::zeroCrossingRate(analysisBuffer);

        return crossings;
    }

    void setWindowSize(uint32 windowSize)
    {
        if(windowSize < tIDLib::MINWINDOWSIZE)
    	{
    		throw std::invalid_argument("window size must be "+std::to_string(tIDLib::MINWINDOWSIZE)+" or greater");
    	}
        this->analysisWindowSize = windowSize;
        resizeBuffers();
        reset();
    }

    uint32 getWindowSize() const
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

        return res;
    }

private:

    void resizeBuffers()
    {
        signalBuffer.resize(analysisWindowSize + blockSize, SampleType{0});
        analysisBuffer.resize(analysisWindowSize, 0.0f);
    }

    void storeAudioBlock (const SampleType* input, size_t n) noexcept
    {
        jassert(n ==  this->blockSize);

        // shift signal buffer contents N positions back
    	for(uint64 i=0; i<analysisWindowSize; ++i)
    		signalBuffer[i] = signalBuffer[i+n];

    	// write new block to end of signal buffer.
    	for(size_t i=0; i<n; ++i)
    		signalBuffer[analysisWindowSize+i] = input[i];
       #if ASYNC_FEATURE_EXTRACTION
        this->lastStoreTime = juce::Time::currentTimeMillis();
       #endif
    }

    //==============================================================================
    double sampleRate = tIDLib::SAMPLERATEDEFAULT;  // x_sr field in Original PD library
    uint32 blockSize = tIDLib::BLOCKSIZEDEFAULT;    // x_n field in Original PD library library
    uint64 analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;   // x_window in Original PD library

    std::vector<SampleType> signalBuffer;
    std::vector<float> analysisBuffer;

   #if ASYNC_FEATURE_EXTRACTION
    uint32 lastStoreTime = juce::Time::currentTimeMillis(); // x_lastDspTime in Original PD library
   #endif
    //==============================================================================
    JUCE_LEAK_DETECTOR (ZeroCrossing)
};

} // namespace tid
