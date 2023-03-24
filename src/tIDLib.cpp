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
            freq = 600.0f * sinh(bark/6.0f);
            break;
        case tIDLib::bark2freqFormula1:
            freq = 53548.0f/(bark*bark - 52.56f*bark + 690.39f);
            break;
        case tIDLib::bark2freqFormula2:
            freq = 1960.0f/(26.81f/(bark+0.53f) - 1.0f);
            freq = (freq<0)?0:freq;
            break;
        default:
            freq = 0.0f;
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
            barkFreq = 6.0f*asinh(freq/600.0f);
            break;
        case tIDLib::freq2barkFormula1:
            barkFreq = 13.0f*atan(0.00076f*freq) + 3.5f*atan(powf((freq/7500.0f), 2));
            break;
        case tIDLib::freq2barkFormula2:
            barkFreq = ((26.81f*freq)/(1960.0f+freq))-0.53f;
            if(barkFreq<2)
                barkFreq += 0.15f*(2-barkFreq);
            else if(barkFreq>20.1f)
                barkFreq += 0.22f*(barkFreq-20.1f);
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

    freq = 700 * (exp(mel/1127.0f) - 1.0f);
//    freq = 700 * (exp(mel/1127.01048f) - 1);
    freq = (freq<0.0f)?0.0f:freq;
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

float euclidDist(t_attributeIdx n, const std::vector<float>& v1, const std::vector<float>& v2, const std::vector<float>& weights, bool sqroot)
{
    float dist = 0.0f;

    for(t_attributeIdx i = 0; i < n; ++i)
    {
        float diff = v1[i] - v2[i];
        dist += diff*diff*weights[i];
    }

    if(sqroot)
        dist = sqrt(dist);

    return(dist);
}

float taxiDist(t_attributeIdx n, const std::vector<float>& v1, const std::vector<float>& v2, const std::vector<float>& weights)
{
    float dist = 0.0f;
    for(t_attributeIdx i = 0; i < n; i++)
        dist += fabs(v1[i] - v2[i]) * weights[i];
    return(dist);
}

float corr(t_attributeIdx n, const std::vector<float>& v1, const std::vector<float>& v2)
{
    float sum1 = 0.0f, sum2 = 0.0f;
    for(t_attributeIdx i = 0; i < n; ++i)
    {
        sum1 += v1[i];
        sum2 += v2[i];
    }

    float mean1 = sum1/n;
    float mean2 = sum2/n;
    std::vector<float> v1centered(n);
    std::vector<float> v2centered(n);

    for(t_attributeIdx i = 0; i < n; ++i)
    {
        v1centered[i] = v1[i] - mean1;
        v2centered[i] = v2[i] - mean2;
    }

    float std1 = 0.0f, std2 = 0.0f;
    for(t_attributeIdx i = 0; i < n; i++)
    {
        std1 += v1centered[i]*v1centered[i];
        std2 += v2centered[i]*v2centered[i];
    }

    std1 = sqrt(std1/n);
    std2 = sqrt(std2/n);

    float corr = 0.0f;
    for(t_attributeIdx i = 0; i < n; i++)
        corr += v1centered[i] * v2centered[i];

    corr /= n;
    corr = corr / (std1 * std2);
    return(corr);
}

void tIDLib_knnInfoBubbleSort(unsigned short int n, std::vector<t_instance>& instances)
{
    for(unsigned short int i = 0; i < n; i++)
    {
        bool flag = false;
        for(unsigned short int j = 0; j < (n-1); j++)
        {
            if(instances[j].knnInfo.safeDist > instances[j+1].knnInfo.safeDist)
            {
                t_knnInfo tmp;

                flag = true;

                tmp = instances[j+1].knnInfo;
                instances[j+1].knnInfo = instances[j].knnInfo;
                instances[j].knnInfo = tmp;
            }
        }

        if(!flag)
            break;
    }
}

void sortKnnInfo(unsigned short int k, t_instanceIdx numInstances, t_instanceIdx prevMatch, std::vector<t_instance>& instances)
{
    std::vector<t_instanceIdx> topMatches(k);

    for(unsigned short int i = 0; i < k; ++i)
    {
        float maxBest = FLT_MAX;
        t_instanceIdx topIdx = 0;

        for(t_instanceIdx j = 0; j < numInstances; ++j)
        {
            if(instances[j].knnInfo.dist < maxBest)
            {
                if(instances[j].knnInfo.idx != prevMatch) // doesn't include previous match - this is good
                {
                    maxBest = instances[j].knnInfo.dist;
                    topIdx = j;
                };
            }
        }

        instances[topIdx].knnInfo.dist = FLT_MAX;

        topMatches[i] = instances[topIdx].knnInfo.idx;
    }

    for(unsigned short int i = 0; i < k; ++i)
    {
        t_knnInfo tmp;

        tmp = instances[i].knnInfo;
        instances[i].knnInfo = instances[topMatches[i]].knnInfo;
        instances[topMatches[i]].knnInfo = tmp;
    }

    // sort the top K matches, because they have the lowest distances in the whole list, but they're not sorted
    tIDLib_knnInfoBubbleSort(k, instances);

    // now, the list passed to the function will have the first k elements in order,
    // and these elements have the lowest distances in the whole list.
}


/* ---------------- END utility functions ---------------------- */

/* ---------------- filterbank functions ---------------------- */

t_binIdx nearestBinIndex(float target, const float *binFreqs, t_binIdx n)
{
    float bestDist = FLT_MAX;
    float dist = 0.0f;
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
// TO-DO: this should probably take in a pointer to x->x_filterbank, resize it to sizeFilterFreqs-2, then write the filter bound freqs in x_filterbank.filterFreqs[0] and [1]. Then x_filterbank can be passed to _createFilterbank with the filter bound freqs known, and no need for a separate x_filterFreqs buffer
// resizes the filterFreqs array and fills it with the Hz values for the Bark filter boundaries. Reports the new number of Bark frequency band boundaries based on the desired spacing. The size of the corresponding filterbank would be sizeFilterFreqs-2
t_filterIdx getBarkBoundFreqs(std::vector<float> &filterFreqs, float spacing, float sr)
{
    if(spacing<0.1f || spacing>6.0f)
        throw std::invalid_argument("Bark spacing must be between 0.1 and 6.0 Barks.");

    float sumBark = 0.0f;
    t_filterIdx sizeFilterFreqs = 0;

    while( (bark2freq(sumBark)<=(sr*0.5f)) && (sumBark<=MAXBARKS) )
    {
        sizeFilterFreqs++;
        sumBark += spacing;
    }

    filterFreqs.resize(sizeFilterFreqs);

    // First filter boundary should be at 0Hz
    filterFreqs[0] = 0.0f;

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


t_filterIdx getMelBoundFreqs(std::vector<float> &filterFreqs, float spacing, float sr)
{
    if(spacing < 5 || spacing > 1000)
       throw std::invalid_argument("Mel spacing must be between 5 and 1000 mels");

    float sumMel = 0.0f;
    t_filterIdx sizeFilterFreqs = 0;

    while( (mel2freq(sumMel)<=(sr*0.5f)) && (sumMel<=MAXMELS) )
    {
        sizeFilterFreqs++;
        sumMel += spacing;
    }

    filterFreqs.resize(sizeFilterFreqs);

    // First filter boundary should be at 0Hz
    filterFreqs[0] = 0.0f;

    // reset the running Bark sum to the first increment past 0 mels
    sumMel = spacing;

    // fill up filterFreqs with the Hz values of all mel range boundaries
    for(t_filterIdx i=1; i<sizeFilterFreqs; ++i)
    {
        filterFreqs[i] = mel2freq(sumMel);
        sumMel += spacing;
    }

    return(sizeFilterFreqs);
}

void createFilterbank(const std::vector<float> &filterFreqs,
                    std::vector<t_filter> &filterbank, t_filterIdx newNumFilters,
                    float window, float sr)
{
    t_binIdx windowHalf = window*0.5;
    t_binIdx windowHalfPlus1 = windowHalf+1;

    if(newNumFilters >= windowHalfPlus1)
        throw std::logic_error("Current filterbank is invalid. For window size N, filterbank size must be less than N/2+1. Change filter spacing or window size accordingly.");

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

    // finally, build the filterbank
    for(t_filterIdx ffi = 1; ffi <= newNumFilters; ++ffi)
    {
        t_binIdx startIdx, peakIdx, finishIdx;

        startIdx = peakIdx = finishIdx = 0;

        startIdx = nearestBinIndex(filterFreqs[ffi-1], binFreqs, windowHalfPlus1);
        peakIdx = nearestBinIndex(filterFreqs[ffi], binFreqs, windowHalfPlus1);
        finishIdx = nearestBinIndex(filterFreqs[ffi+1], binFreqs, windowHalfPlus1);

        jassert(startIdx<=finishIdx);
        jassert(peakIdx<=finishIdx);
        jassert(startIdx<=peakIdx);

        // resize this filter
        t_binIdx filterWidth = finishIdx-startIdx + 1;
        filterWidth = (filterWidth<1)?1:filterWidth;

        filterbank[ffi-1].size = filterWidth; // store the sizes for freeing memory later
        filterbank[ffi-1].filter.resize(filterWidth);

        // initialize this filter
        for(t_binIdx j = 0; j < filterWidth; ++j)
            filterbank[ffi-1].filter[j] = 0.0f;

        // some special cases for very narrow filter widths
        switch(filterWidth)
        {
            case 1:
                filterbank[ffi-1].filter[0] = 1.0f;
                break;
            // original wbrent comment:
            // no great way to do a triangle with a filter width of 2, so might as well average
            case 2:
                filterbank[ffi-1].filter[0] = 0.5f;
                filterbank[ffi-1].filter[1] = 0.5f;
                break;

            // with 3 and greater, we can use our ramps
            default:
                t_binIdx upN = peakIdx-startIdx+1;
                std::vector<float> upRamp(upN);
                linspace(upRamp, 0.0f, 1.0f);

                t_binIdx downN = finishIdx-peakIdx+1;
                std::vector<float> downRamp(downN);
                linspace(downRamp, 1.0f, 0.0f);

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
                    if(filterbank[ffi-1].filter[fj] < 0.0f)
                        filterbank[ffi-1].filter[fj] = 0.0f;

                    if(filterbank[ffi-1].filter[fj] > 1.0f)
                        filterbank[ffi-1].filter[fj] = 1.0f;
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

    // free local memory
    std::vector<float>().swap(binFreqs); // Swaps binfreqs with an empty array (btter than clear())
}

void specFilterBands(t_binIdx n, t_filterIdx numFilters, float *spectrum, std::vector<t_filter> &filterbank, bool normalize)
{
    float totalEnergy = 0;

    for(t_filterIdx i=0; i<numFilters; ++i)
    {
        float smoothedSpec = 0.0f;

        for(t_binIdx j=filterbank[i].indices[0]; j<=filterbank[i].indices[1]; ++j)
                smoothedSpec += spectrum[j];

        smoothedSpec /= filterbank[i].size;
        totalEnergy += smoothedSpec;

        filterbank[i].tmpValue = smoothedSpec; // tmpVal is the smoothedSpec for each filter bank
    };

    // Check that the spectrum window size N is larger than the number of filters, otherwise we'll be writing to invalid memory indices
    if(n>=numFilters)
    {
        for(t_filterIdx si=0; si<numFilters; ++si)
            if(normalize)
                spectrum[si] = filterbank[si].tmpValue/totalEnergy; // tmpValue is the smoothedSpec
            else
                spectrum[si] = filterbank[si].tmpValue; // tmpValue is the smoothedSpec
    }
    else
    {
        for(t_filterIdx si=0; si<n; ++si)
            if(normalize)
                spectrum[si] = filterbank[si].tmpValue/totalEnergy; // tmpValue is the smoothedSpec
            else
                spectrum[si] = filterbank[si].tmpValue; // tmpValue is the smoothedSpec
    }
}

void filterbankMultiply(float *spectrum, bool normalize, bool filterAvg, std::vector<t_filter> &filterbank, t_filterIdx numFilters)
{
    float sumSum = 0;
    for(t_filterIdx i=0; i<numFilters; ++i)
    {
        float sum = 0.0f;
        t_binIdx j, k;

        for(j=filterbank[i].indices[0], k=0; j<=filterbank[i].indices[1]; ++j, ++k)
            sum += spectrum[j] * filterbank[i].filter[k];

        k = filterbank[i].size;
        if(filterAvg)
            sum /= k;

        filterbank[i].tmpValue = sum;  // get the total power.  another weighting might be better.

        sumSum += sum;  // normalize so power in all bands sums to 1
    };

    if(normalize)
    {
        // prevent divide by 0
        if(sumSum==0)
            sumSum=1;
        else
            sumSum = 1.0f/sumSum; // take the reciprocal here to save a divide below
    }
    else
        sumSum=1.0f;

    for(t_binIdx si=0; si<numFilters; ++si)
        spectrum[si] = filterbank[si].tmpValue * sumSum;
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

void veclog(t_binIdx n, float *input)  // compute logarithm of array elements
{
    while (n--)
    {
        // if to protect against log(0)
        if(*input==0.0f)
            *input = 0.0f;
        else
            *input = log(*input);

        input++;
    };
}

void veclog(t_binIdx n, std::vector<float> &input)
{
    float *inputPtr = &(input[0]);
    veclog(n,inputPtr);
}

/* ---------------- END dsp utility functions ---------------------- */



/* ---------------- windowing buffer functions ---------------------- */

void initBlackmanWindow(std::vector<float> &window)
{
    unsigned long int n = window.size();
    for(unsigned long int i = 0; i < n; ++i)
        window[i] = 0.42f - (0.5f * cos(2.0f*M_PI*i/n)) + (0.08f * cos(4.0f*M_PI*i/n));
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
        window[i] = 0.5f - (0.46f * cos(2.0f*M_PI*i/n));
}

void initHannWindow(std::vector<float> &window)
{
    unsigned long int n = window.size();
    for(unsigned long int i = 0; i < n; ++i)
        window[i]  = 0.5f * (1.0f - cos(2.0f*M_PI*i/n));
}
/* ---------------- END windowing buffer functions ---------------------- */

}
