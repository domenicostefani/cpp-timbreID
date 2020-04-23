/*

TimbreID Library - main file
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 12th March 2020 (domenico.stefani96@gmail.com)

**** Original LICENCE disclaimer ahead ****

Copyright 2009 William Brent

This file is part of timbreID.

timbreID is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

timbreID is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#include "tIDLib.hpp"

#include <JuceHeader.h>
#include <cmath>
#include <vector>
#include <cfloat>   // FLT_MAX
#include <climits>  // ULONG_MAX

namespace tIDLib
{

/* ---------------- conversion functions ---------------------- */
float freq2bin(float freq, float n, float sr)
{
    float bin;

    bin = freq*n/sr;

    return(bin);
}

float bin2freq(float bin, float n, float sr)
{
    float freq;

    freq = bin*sr/n;

    return(freq);
}

float bark2freq(float bark)
{
    float freq;
    tIDLib::t_bark2freqFormula formula;

    // formula 2 was used in timbreID 0.6
    formula = tIDLib::bark2freqFormula0;

    switch(formula)
    {
        case tIDLib::bark2freqFormula0:
            freq = 600.0 * sinh(bark/6.0);
            break;
        case tIDLib::bark2freqFormula1:
            freq = 53548.0/(bark*bark - 52.56*bark + 690.39);
            break;
        case tIDLib::bark2freqFormula2:
            freq = 1960/(26.81/(bark+0.53) - 1);
            freq = (freq<0)?0:freq;
            break;
        default:
            freq = 0;
    }

    return(freq);
}

float freq2bark(float freq)
{
    float barkFreq;
    tIDLib::t_freq2barkFormula formula;

    formula = tIDLib::freq2barkFormula0;

    switch(formula)
    {
        case tIDLib::freq2barkFormula0:
            barkFreq = 6.0*asinh(freq/600.0);
            break;
        case tIDLib::freq2barkFormula1:
            barkFreq = 13*atan(0.00076*freq) + 3.5*atan(powf((freq/7500), 2));
            break;
        case tIDLib::freq2barkFormula2:
            barkFreq = ((26.81*freq)/(1960+freq))-0.53;
            if(barkFreq<2)
                barkFreq += 0.15*(2-barkFreq);
            else if(barkFreq>20.1)
                barkFreq += 0.22*(barkFreq-20.1);
            break;
        default:
            barkFreq = 0;
            break;
    }

    barkFreq = (barkFreq<0)?0:barkFreq;

    return(barkFreq);
}

float freq2mel(float freq)
{
    float mel;

    mel = 1127*log(1+(freq/700));
    mel = (mel<0)?0:mel;
    return(mel);
}

float mel2freq(float mel)
{
    float freq;

    freq = 700 * (exp(mel/1127) - 1);
//    freq = 700 * (exp(mel/1127.01048) - 1);
    freq = (freq<0)?0:freq;
    return(freq);
}
/* ---------------- END conversion functions ---------------------- */

/* ---------------- utility functions ---------------------- */
void linspace(std::vector<float> &ramp, float start, float finish)
{
    float diffInc = (finish-start)/(float)(ramp.size()-1);

    ramp[0] = start;
    for(t_binIdx i = 1; i < ramp.size(); ++i)
        ramp[i] = ramp[i-1] + diffInc;
}

signed char signum(float input)
{
    signed char sign = 0;

    if(input > 0)
        sign = 1;
    else if(input < 0)
        sign = -1;
    else
        sign = 0;

    return(sign);
}
/* ---------------- END utility functions ---------------------- */

/* ---------------- filterbank functions ---------------------- */

t_binIdx nearestBinIndex(float target, const float *binFreqs, t_binIdx n)
{
    float bestDist = FLT_MAX;
    float dist = 0.0;
    t_binIdx bin = 0;

    for(t_binIdx i=0; i<n; ++i)
    {
        if((dist=fabs(binFreqs[i]-target)) < bestDist)
        {
            bestDist = dist;
            bin = i;
        }
    }

    return(bin);
}

t_binIdx nearestBinIndex(float target, const std::vector<float> &binFreqs, t_binIdx n)
{
    jassert(binFreqs.size() >= n);
    return nearestBinIndex(target, &binFreqs[0], n);
}

//Original wbrent comment
// TODO: this should probably take in a pointer to x->x_filterbank, resize it to sizeFilterFreqs-2, then write the filter bound freqs in x_filterbank.filterFreqs[0] and [1]. Then x_filterbank can be passed to _createFilterbank with the filter bound freqs known, and no need for a separate x_filterFreqs buffer
// resizes the filterFreqs array and fills it with the Hz values for the Bark filter boundaries. Reports the new number of Bark frequency band boundaries based on the desired spacing. The size of the corresponding filterbank would be sizeFilterFreqs-2
t_filterIdx getBarkBoundFreqs(std::vector<float> &filterFreqs, float spacing, float sr)
{
    if(spacing<0.1 || spacing>6.0)
        throw std::invalid_argument("Bark spacing must be between 0.1 and 6.0 Barks.");

    float sumBark = 0.0;
    t_filterIdx sizeFilterFreqs = 0;

    while( (bark2freq(sumBark)<=(sr*0.5)) && (sumBark<=MAXBARKS) )
    {
        sizeFilterFreqs++;
        sumBark += spacing;
    }

    filterFreqs.resize(sizeFilterFreqs);

    // First filter boundary should be at 0Hz
    filterFreqs[0] = 0.0;

    // reset the running Bark sum to the first increment past 0 Barks
    sumBark = spacing;

    // fill up filterFreqs with the Hz values of all Bark range boundaries
    for(t_filterIdx i=1; i<sizeFilterFreqs; ++i)
    {
        filterFreqs[i] = bark2freq(sumBark);
        sumBark += spacing;
    }

    return(sizeFilterFreqs);
}



void createFilterbank(const std::vector<float> &filterFreqs,
                    std::vector<t_filter> &filterbank, t_filterIdx newNumFilters,
                    float window, float sr)
{
    t_binIdx windowHalf = window*0.5;
    t_binIdx windowHalfPlus1 = windowHalf+1;

    // create local memory
    std::vector<float> binFreqs(windowHalfPlus1);

    // resize filterbank vector
    filterbank.resize(newNumFilters);

    for(t_filterIdx i=0; i<newNumFilters; ++i)
    {
        // initialize indices
        for(unsigned char j = 0; j < 2; ++j)
            filterbank[i].indices[j] = 0;

        // initialize filterbank sizes
        filterbank[i].size = 0;
    }

    // first, find the actual freq for each bin based on current window size
    for(t_binIdx bfi = 0; bfi < windowHalfPlus1; ++bfi)
        binFreqs[bfi] = bin2freq(bfi, window, sr);


    if(newNumFilters >= windowHalfPlus1)
    {
        throw std::logic_error("Current filterbank is invalid. For window size N, filterbank size must be less than N/2+1. Change filter spacing or window size accordingly.");
    }
    else
    {
        // finally, build the filterbank
        for(t_filterIdx ffi = 1; ffi <= newNumFilters; ++ffi)
        {
            t_binIdx startIdx, peakIdx, finishIdx;

            startIdx = peakIdx = finishIdx = 0;

            startIdx = nearestBinIndex(filterFreqs[ffi-1], binFreqs, windowHalfPlus1);
            peakIdx = nearestBinIndex(filterFreqs[ffi], binFreqs, windowHalfPlus1);
            finishIdx = nearestBinIndex(filterFreqs[ffi+1], binFreqs, windowHalfPlus1);

            jassert(startIdx<=finishIdx);   //TODO Remove the equal if it's bad
            jassert(peakIdx<=finishIdx);
            jassert(startIdx<=peakIdx);

            // resize this filter
            t_binIdx filterWidth = finishIdx-startIdx + 1;
            filterWidth = (filterWidth<1)?1:filterWidth;

            filterbank[ffi-1].size = filterWidth; // store the sizes for freeing memory later
            filterbank[ffi-1].filter.resize(filterWidth);

            // initialize this filter
            for(t_binIdx j = 0; j < filterWidth; ++j)
                filterbank[ffi-1].filter[j] = 0.0;

            // some special cases for very narrow filter widths
            switch(filterWidth)
            {
                case 1:
                    filterbank[ffi-1].filter[0] = 1.0;
                    break;
                // original wbrent comment:
                // no great way to do a triangle with a filter width of 2, so might as well average
                case 2:
                    filterbank[ffi-1].filter[0] = 0.5;
                    filterbank[ffi-1].filter[1] = 0.5;
                    break;

                // with 3 and greater, we can use our ramps
                default:
                    t_binIdx upN = peakIdx-startIdx+1;
                    std::vector<float> upRamp(upN);
                    linspace(upRamp, 0.0, 1.0);

                    t_binIdx downN = finishIdx-peakIdx+1;
                    std::vector<float> downRamp(downN);
                    linspace(downRamp, 1.0, 0.0);

                    t_binIdx fj;
                    // copy into filterbank[i-1].filter
                    for(fj = 0; fj < upN; ++fj)
                        filterbank[ffi-1].filter[fj] = upRamp[fj];

                    // start at k=1 because k=0 will be the peak (i.e., 1.0)
                    for(t_binIdx k = 1; k < downN; ++fj, ++k)
                        filterbank[ffi-1].filter[fj] = downRamp[k];

                    // clip the triangle within 0 and 1, just in case
                    for(t_binIdx fj = 0; fj < filterWidth; ++fj)
                    {
                        if(filterbank[ffi-1].filter[fj] < 0.0)
                            filterbank[ffi-1].filter[fj] = 0.0;

                        if(filterbank[ffi-1].filter[fj] > 1.0)
                            filterbank[ffi-1].filter[fj] = 1.0;
                    }

                    // free up memory (Does this actually )
                    std::vector<float>().swap(upRamp); // Swaps with empty vec
                    std::vector<float>().swap(downRamp); // Swaps with empty vec
                    break;
            };

            filterbank[ffi-1].indices[0] = startIdx;
            filterbank[ffi-1].indices[1] = finishIdx;
            filterbank[ffi-1].filterFreqs[0] = binFreqs[startIdx];
            filterbank[ffi-1].filterFreqs[1] = binFreqs[finishIdx];
        }
    }

    // free local memory
    std::vector<float>().swap(binFreqs); // Swaps binfreqs with an empty array (btter than clear())
}

void specFilterBands(t_binIdx n, t_filterIdx numFilters, float *spectrum, const std::vector<t_filter> &filterbank, bool normalize)
{
    float smoothedSpec[numFilters], totalEnergy;

    totalEnergy = 0;

    for(t_filterIdx i=0; i<numFilters; ++i)
    {
        smoothedSpec[i] = 0.0;

        for(t_binIdx j=filterbank[i].indices[0]; j<=filterbank[i].indices[1]; ++j)
                smoothedSpec[i] += spectrum[j];

        smoothedSpec[i] /= filterbank[i].size;

        totalEnergy += smoothedSpec[i];
    };

    // Check that the spectrum window size N is larger than the number of filters, otherwise we'll be writing to invalid memory indices
    if(n>=numFilters)
    {
        for(t_filterIdx si=0; si<numFilters; ++si)
            if(normalize)
                spectrum[si] = smoothedSpec[si]/totalEnergy;
            else
                spectrum[si] = smoothedSpec[si];
    }
    else
    {
        for(t_filterIdx si=0; si<n; ++si)
            if(normalize)
                spectrum[si] = smoothedSpec[si]/totalEnergy;
            else
                spectrum[si] = smoothedSpec[si];
    }
}

void filterbankMultiply(float *spectrum, bool normalize, bool filterAvg, const std::vector<t_filter> &filterbank, t_filterIdx numFilters)
{
    // create local memory
    std::vector<float> filterPower;
    filterPower.resize(numFilters);

    float sumSum = 0;
    for(t_filterIdx i=0; i<numFilters; ++i)
    {
        float sum = 0.0;
        t_binIdx j, k;

        for(j=filterbank[i].indices[0], k=0; j<=filterbank[i].indices[1]; ++j, ++k)
            sum += spectrum[j] * filterbank[i].filter[k];

        k = filterbank[i].size;
        if(filterAvg)
            sum /= k;

        filterPower[i] = sum;  // get the total power.  another weighting might be better.

        sumSum += sum;  // normalize so power in all bands sums to 1
    };

    if(normalize)
    {
        // prevent divide by 0
        if(sumSum==0)
            sumSum=1;
        else
            sumSum = 1.0/sumSum; // take the reciprocal here to save a divide below
    }
    else
        sumSum=1.0;

    for(t_binIdx si=0; si<numFilters; ++si)
        spectrum[si] = filterPower[si] * sumSum;
}

/* ---------------- END filterbank functions ---------------------- */


/* ---------------- dsp utility functions ---------------------- */

void peakSample(std::vector<float> &input, unsigned long int *peakIdx, float *peakVal)
{
    *peakVal = -FLT_MAX;
    *peakIdx = ULONG_MAX;

    for(unsigned long int i = 1; i < input.size(); ++i)
    {
        if(fabs(input[i]) > *peakVal)
        {
            *peakIdx = i;
            *peakVal = fabs(input[i]);
        }
    }
}

//TODO: check why sometimes the returned index is greater than the max (Original pd module seems to do that too)
unsigned long int findAttackStartSamp(std::vector<float> &input, float sampDeltaThresh, unsigned short int numSampsThresh)
{
    unsigned long int startSamp = ULONG_MAX;

    unsigned long int i = input.size();
    while(i--)
    {
        if(fabs(input[i]) <= sampDeltaThresh)
        {
            unsigned short int sampCount = 1;
            unsigned long int j = i;

            while(j--)
            {
                if(fabs(input[j]) <= sampDeltaThresh)
                {
                    sampCount++;

                    if(sampCount >= numSampsThresh)
                    {
                        startSamp = j;
                        return(startSamp);
                    }
                }
                else
                    break;
            }
        }
    }

    return(startSamp);
}

// this could also return the location of the zero crossing
unsigned int zeroCrossingRate(const std::vector<float>& buffer)
{
    jassert(buffer.size() > 0);

    unsigned int crossings = 0;

    for(size_t sample = 1; sample < buffer.size(); ++sample)
        crossings += abs(signum(buffer[sample]) - signum(buffer[sample-1]));

    crossings *= 0.5f;

    return crossings;
}

void power(t_binIdx n, void *fftw_out, float *powBuf)
{
    fftwf_complex *fftw_out_local = (fftwf_complex *)fftw_out;

    while(n--)
        powBuf[n] = (fftw_out_local[n][0] * fftw_out_local[n][0]) + (fftw_out_local[n][1] * fftw_out_local[n][1]);

}

void mag(t_binIdx n, float *input)
{
    while(n--)
    {
        *input = sqrt(*input);
        input++;
    }
}

/* ---------------- END dsp utility functions ---------------------- */



/* ---------------- windowing buffer functions ---------------------- */

void initBlackmanWindow(std::vector<float> &window)
{
    unsigned long int n = window.size();
    for(unsigned long int i = 0; i < n; ++i)
        window[i] = 0.42 - (0.5 * cos(2*M_PI*i/n)) + (0.08 * cos(4*M_PI*i/n));
}

void initCosineWindow(std::vector<float> &window)
{
    unsigned long int n = window.size();
    for(unsigned long int i = 0; i < n; ++i)
        window[i] = sin(M_PI*i/n);
}

void initHammingWindow(std::vector<float> &window)
{
    unsigned long int n = window.size();
    for(unsigned long int i = 0; i < n; ++i)
        window[i] = 0.5 - (0.46 * cos(2*M_PI*i/n));
}

void initHannWindow(std::vector<float> &window)
{
    unsigned long int n = window.size();
    for(unsigned long int i = 0; i < n; ++i)
        window[i]  = 0.5 * (1 - cos(2*M_PI*i/n));
}
/* ---------------- END windowing buffer functions ---------------------- */

}
