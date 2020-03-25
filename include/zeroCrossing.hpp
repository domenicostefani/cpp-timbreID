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
#include <iostream>

namespace tid   /* TimbreID namespace*/
{

template <typename SampleType>
class ZeroCrossing
{
public:

    //==============================================================================
    /** Creates a zeroCrossing module with default parameters. */
    ZeroCrossing(){ resizeBuffers(); reset(); }

    /** Creates a copy of another zeroCrossing module. */
    ZeroCrossing (const ZeroCrossing&) = default;

    /** Move constructor */
    ZeroCrossing (ZeroCrossing&&) = default;

    //==============================================================================
    /** Initialization of the module */
    void prepare (double sampleRate, uint32 argblockSize) noexcept
    {
        if((sampleRate != this->sampleRate) || (argblockSize != this->blockSize))
        {
            this->sampleRate = sampleRate;
            this->blockSize = argblockSize;
            resizeBuffers();
        }
        reset();
    }

    /** Resets the processing pipeline. */
    void reset() noexcept
    {
        std::fill(signalBuffer.begin(), signalBuffer.end(), SampleType{0});
        std::fill(analysisBuffer.begin(), analysisBuffer.end(), SampleType{0});
        this->lastStoreTime = juce::Time::currentTimeMillis();
    }

    //==============================================================================
    template <typename ProcessContext>
    void store (const ProcessContext& context) noexcept
    {
        static_assert (std::is_same<typename ProcessContext::SampleType, SampleType>::value,
                       "The sample-type of the zeroCrossing module must match the sample-type supplied to this store callback");

        storeInternal<ProcessContext> (context);
    }

    uint32 countCrossings(){
        uint32 currentTime = getTimeSince(this->lastStoreTime);
        if(currentTime > blockSize*sampleRate)
            throw std::logic_error("Clock measure may have overflowed");

        uint32 bangSample = roundf((currentTime / 1000.0) * this->sampleRate);

        // If the period was too long, we cap the maximum number of samples, which is @blockSize
        if(bangSample >= this->blockSize)
            bangSample = this->blockSize - 1;   // TODO: check corner case

        for(uint64 i = 0, j = bangSample; i < this->analysisWindowSize; ++i, ++j)
            this->analysisBuffer[i] = this->signalBuffer[j];

        uint32 crossings = 0;

        jassert(this->analysisBuffer.size() == this->analysisWindowSize);
        crossings = tIDLib::zeroCrossingRate(analysisBuffer);

        return crossings;
    }

private:
    /* Utilities --------*/

    uint32 getTimeSince(uint32 lastTime)
    {
        return (juce::Time::currentTimeMillis() - lastTime);
    }

    /* END utilities ----*/

    void resizeBuffers()
    {
        signalBuffer.resize(analysisWindowSize + blockSize,SampleType{0});
        analysisBuffer.resize(analysisWindowSize,SampleType{0});
    }

    // TODO: may remove
    void bypassBlock (const SampleType* input, SampleType* output, size_t n) noexcept
    {
        for (size_t i = 0 ; i < n; ++i)
            output[i] = input[i];
    }

    void storeBlock (const SampleType* input, size_t n) noexcept
    {
        // shift signal buffer contents N positions back
    	for(uint64 i=0; i<analysisWindowSize; ++i)
    		signalBuffer[i] = signalBuffer[i+n];

    	// write new block to end of signal buffer.
    	for(size_t i=0; i<n; ++i)
    		signalBuffer[analysisWindowSize+i] = input[i];

        this->lastStoreTime = juce::Time::currentTimeMillis();
    }

    template <typename ProcessContext>
    void storeInternal (const ProcessContext& context) noexcept
    {
        auto&& inputBlock  = context.getInputBlock();
        auto&& outputBlock = context.getOutputBlock();

        // This class can only process mono signals. Use the ProcessorDuplicator class
        // to apply this filter on a multi-channel audio stream.
        jassert (inputBlock.getNumChannels()  == 1);
        jassert (outputBlock.getNumChannels() == 1);

        auto n = inputBlock.getNumSamples();
        jassert(n ==  this->blockSize);
        auto* src = inputBlock .getChannelPointer (0);
        auto* dst = outputBlock.getChannelPointer (0);

        // Copy the current input to the output since the block does not modify it
        bypassBlock(src,dst,n);

        // Store the current block in the signal buffer
        storeBlock(src,n);
    }

    //==============================================================================
    double sampleRate = tIDLib::SAMPLERATEDEFAULT;  // x_sr field in Original PD library
    uint32 blockSize = tIDLib::BLOCKSIZEDEFAULT;    // x_n field in Original PD library
    int16 overlap = tIDLib::OVERLAPDEFAULT;         // x_overlap field in Original PD library
    uint64 analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;   // x_window in Original PD library

    std::vector<SampleType> signalBuffer;
    std::vector<SampleType> analysisBuffer;

    uint32 lastStoreTime = juce::Time::currentTimeMillis(); // x_lastDspTime in Original PD library

    //==============================================================================
    JUCE_LEAK_DETECTOR (ZeroCrossing)
};

} // namespace tid
