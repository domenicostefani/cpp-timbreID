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

#include "tIDLib.hpp"
#include <vector>

#include <climits>  // ULONG_MAX

namespace tid   /* TimbreID namespace*/
{

template <typename SampleType>
class AttackTime
{
public:

    //==============================================================================
    /** Creates a AttackTime module with default parameters. */
    AttackTime(){ resizeAllBuffers(); reset(); }

    /** Creates a AttackTime module with custom analysis window size. */
    AttackTime(unsigned long int windowSize)
    {
        this->analysisWindowSize = windowSize;
        resizeAllBuffers();
        reset();
    }

    /** Creates a copy of another AttackTime module. */
    AttackTime (const AttackTime&) = default;

    /** Move constructor */
    AttackTime (AttackTime&&) = default;

    //==============================================================================
    /** Initialization of the module */
    void prepare (double sampleRate, uint32 blockSize) noexcept
    {
        if((sampleRate != this->sampleRate) || (blockSize != this->blockSize))
        {
            this->sampleRate = sampleRate;
            this->blockSize = blockSize;
            resizeAllBuffers();
        }
        reset();
    }

    /** Resets the processing pipeline. */
    void reset() noexcept
    {
        std::fill(signalBuffer.begin(), signalBuffer.end(), SampleType{0});
        std::fill(analysisBuffer.begin(), analysisBuffer.end(), 0.0f);
        std::fill(searchBuffer.begin(), searchBuffer.end(), 0.0f);
       #if ASYNC_FEATURE_EXTRACTION
        this->lastStoreTime = juce::Time::currentTimeMillis();
       #endif
    }

    template <typename OtherSampleType>
    void store (AudioBuffer<OtherSampleType>& buffer, short channel)
    {
        static_assert (std::is_same<OtherSampleType, SampleType>::value,
                       "The sample-type of the AttackTime module must match the sample-type supplied to this store callback");

        short numChannels = buffer.getNumChannels();
        jassert(channel < numChannels);
        jassert(channel >= 0);

        if(channel < 0 || channel >= numChannels)
            throw std::invalid_argument("Channel index has to be between 0 and " + std::to_string(numChannels));
        storeAudioBlock(buffer.getReadPointer(channel), buffer.getNumSamples());
    }
    //==============================================================================

    void compute(unsigned long int* rPeakSampIdx, unsigned long int* rAttackStartIdx, float *rAttackTime)
    {
       #if ASYNC_FEATURE_EXTRACTION
        uint32 currentTime = tid::Time::getTimeSince(this->lastStoreTime);
        // if(currentTime > blockSize*sampleRate)
        //     throw std::logic_error("Clock measure may have overflowed");

        uint32 bangSample = roundf((currentTime / 1000.0) * this->sampleRate);

        // If the period was too long, we cap the maximum number of samples, which is @blockSize
        if(bangSample >= this->blockSize)
            bangSample = this->blockSize - 1;

        // took a while to get this calculation right, but it seems correct now. remember that bangSample is always between 0 and 63 (or blockSize-1), and finding startSample within x_signalBuffer involves a few other steps. (wbrent original comment)
        unsigned long int startSample = (this->maxSearchRange + this->blockSize) - bangSample - analysisWindowSize - 1;
       #else
        unsigned long int startSample = 0;
       #endif

        // construct analysis window
        for(unsigned long int i = 0, j = startSample; i < analysisWindowSize; ++i, ++j)
            this->analysisBuffer[i] = (float)this->signalBuffer[j];

        unsigned long int peakSampIdx = 0;
        float peakSampVal = 0.0;
        tIDLib::peakSample(this->analysisBuffer, &peakSampIdx, &peakSampVal);

        peakSampIdx += startSample; // add startSample back so we can find the peak sample index relative to x_signalBuffer (wbrent original comment)

        unsigned long int i = this->maxSearchRange;
        unsigned long int j = peakSampIdx;

        while(i--)
        {
            if(j == 0)
                this->searchBuffer[i] = this->signalBuffer[j];
            else
            {
                this->searchBuffer[i] = this->signalBuffer[j];
                j--;
            }
        }

        // send searchBuffer to routine to find the point where sample magnitude is below x_sampMagThresh for at least x_numSampsThresh samples (wbrent original comment)
        unsigned long int attackStartIdx = tIDLib::findAttackStartSamp(searchBuffer, sampMagThresh, numSampsThresh);

        float attackTime = 0.0;
        // if the index returned is ULONG_MAX, the search failed
        if(attackStartIdx == ULONG_MAX)
        	attackTime = -1.0;
        else
        {
            // attack duration in samples is the end of buffer index (where the peak sample was) minus the start index
            attackTime = (this->maxSearchRange - attackStartIdx)/this->sampleRate;
            attackTime *= 1000.0; // convert seconds to milliseconds
            // overwrite attackStartIdx to be the index relative to the entire table
            attackStartIdx = peakSampIdx - (this->maxSearchRange - attackStartIdx);
        }

        // Output the values just computed ------------------------
        *rPeakSampIdx = peakSampIdx;
        *rAttackStartIdx = attackStartIdx;
        *rAttackTime = attackTime;
        //---------------------------------------------------------
    }

    void setWindowSize(uint32 windowSize)
    {
        this->analysisWindowSize = windowSize;
        resizeAnalysisBuffer();
        reset();
    }

    uint32 getWindowSize() const
    {
        return this->analysisWindowSize;
    }

    /**
     * Specify the maximum time in milliseconds before the peak sample of an
     * attack to search for the initial onset. This determines the size of the
     * internal buffer and is separate from the analysis window size
     * (default: 2000ms)
    */
    void setMaxSearchRange(float rangeMillis)
    {
    	rangeMillis = (rangeMillis<5.0)?5.0:rangeMillis;
    	unsigned long int newRange = roundf((rangeMillis/1000.0) * this->sampleRate);

    	this->maxSearchRange = newRange;

        resizeAllBuffers();
        reset();
    }

    unsigned long int getMaxSearchRange() const
    {
        return this->maxSearchRange;
    }

    void setSampMagThresh(float thresh)
    {
        if(thresh < 0.0)
            throw std::invalid_argument("Threshold has to be a positive value");

    	this->sampMagThresh = thresh;
    }

    float getSampMagThresh() const
    {
        return this->sampMagThresh;
    }

    void setNumSampsThresh(unsigned long int thresh)
    {
        if(thresh < 0.0 || thresh > this->maxSearchRange)
            throw std::invalid_argument("Threshold has to be between 0 and the maximumSearchRange ("+std::to_string(this->maxSearchRange)+")");

    	this->numSampsThresh = thresh;
    }

    unsigned long int getNumSampsThresh() const
    {
        return this->numSampsThresh;
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

        res += "\nMinimum sample threshold: ";
        res += std::to_string(this->numSampsThresh);

        res += "\nSample magnitude threshold: ";
        res += std::to_string(this->sampMagThresh);

        res += "\nMax Search Range: ";
        res += std::to_string(this->maxSearchRange);

        return res;
    }

private:

    void resizeAnalysisBuffer()
    {
        analysisBuffer.resize(analysisWindowSize, 0.0f);
    }

    void resizeAllBuffers()
    {
        signalBuffer.resize(maxSearchRange + blockSize, SampleType{0});
        searchBuffer.resize(maxSearchRange, 0.0f);
        resizeAnalysisBuffer();
    }

    void storeAudioBlock (const SampleType* input, size_t n) noexcept
    {
        jassert(n ==  this->blockSize);

        // shift signal buffer contents N positions back
    	for(uint64 i=0; i<maxSearchRange; ++i)
    		signalBuffer[i] = signalBuffer[i+n];

    	// write new block to end of signal buffer.
    	for(size_t i=0; i<n; ++i)
    		signalBuffer[maxSearchRange+i] = input[i];

       #if ASYNC_FEATURE_EXTRACTION
        this->lastStoreTime = juce::Time::currentTimeMillis();
       #endif
    }

    //==============================================================================
    double sampleRate = tIDLib::SAMPLERATEDEFAULT;  // x_sr field in Original PD library
    uint32 blockSize = tIDLib::BLOCKSIZEDEFAULT;    // x_n field in Original PD library library
    uint64 analysisWindowSize = tIDLib::WINDOWSIZEDEFAULT;   // x_window in Original PD library

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
    uint32 lastStoreTime = juce::Time::currentTimeMillis(); // x_lastDspTime in Original PD library
   #endif

    //==============================================================================
    JUCE_LEAK_DETECTOR (AttackTime)
};

} // namespace tid
