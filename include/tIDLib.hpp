#pragma once

#include <vector>
#include "fftw3.h"
#include <cmath>
#include <string>

typedef unsigned long int t_binIdx; // 0 to 18,446,744,073,709,551,615
typedef unsigned short int t_filterIdx;
typedef unsigned int t_attributeIdx; // gets us over 4 billion attributes. should be plenty indices to instances in database. members of a cluster. non-negative only
typedef unsigned int t_instanceIdx;

#define TIDVERSION "0.8.2C"

// choose either FFTW_MEASURE or FFTW_ESTIMATE here.
#define FFTWPLANNERFLAG (FFTW_ESTIMATE | FFTW_CONSERVE_MEMORY)

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

namespace tIDLib
{
constexpr unsigned int BLOCKSIZEDEFAULT = 64;
constexpr float MINBARKSPACING = 0.1;
constexpr float MAXBARKSPACING = 6.0;
constexpr float BARKSPACINGDEFAULT = 0.5;
constexpr float MINMELSPACING = 5.0;
constexpr float MAXMELSPACING = 1000.0;
constexpr float MELSPACINGDEFAULT = 100.0;
constexpr unsigned long int WINDOWSIZEDEFAULT = 1024;
constexpr unsigned long int MINWINDOWSIZE = 4;
constexpr unsigned long int  SAMPLERATEDEFAULT = 44100;
constexpr float MAXBARKS = 26.0;
constexpr float MAXBARKFREQ = 22855.4;
constexpr float MAXMELFREQ = 22843.6;
constexpr float MAXMELS = 3962.0;
constexpr int NUMWEIGHTPOINTS = 29;
constexpr int MAXTIDTEXTSTRING = 100000;

const std::string LOG_PATH = "/tmp/";
const std::string LOG_EXTENSION = "log";

enum t_bark2freqFormula
{
	bark2freqFormula0 = 0,
	bark2freqFormula1,
	bark2freqFormula2
};

enum t_freq2barkFormula
{
	freq2barkFormula0 = 0,
	freq2barkFormula1,
	freq2barkFormula2
};

enum WindowFunctionType
{
    rectangular = 0,
    blackman,
    cosine,
    hamming,
    hann
};

typedef struct filter
{
	std::vector<float> filter;
	t_binIdx size;
	t_binIdx indices[2];
	float filterFreqs[2];

    float tmpValue; // Temp value used to avoid memory allocation in real time-operations
} t_filter;

/**
 *  State of the triangular filters (enabled or disabled)
*/
enum FilterState
{
    filterEnabled = 0,    // Using triangular filterbank.
    filterDisabled        // Averaging energy in spectrum bands.
};

/**
 * Type of operation performed between the energies of each filter
 * (Sum or sample mean)
*/
enum FilterOperation
{
    sumFilterEnergy = 0,
    averageFilterEnergy
};

/**
 * Spectrum type used (power spectrum or magnitude spectrum)
*/
enum SpectrumType
{
    powerSpectrum = 0,
    magnitudeSpectrum
};


/**
 * Cepstrum type used (power cepstrum or magnitude cepstrum)
*/
enum CepstrumType
{
    powerCepstrum = 0,
    magnitudeCepstrum
};


typedef struct knnInfo
{
    float dist;
	float safeDist;
    t_instanceIdx idx;
    t_instanceIdx cluster;
} t_knnInfo;

typedef struct instance
{
    std::vector<float> data;
    t_attributeIdx length;
    t_instanceIdx clusterMembership;
    t_knnInfo knnInfo;
} t_instance;

typedef struct cluster
{
    std::vector<t_instanceIdx> members;
    t_instanceIdx numMembers;
    unsigned int votes;
} t_cluster;

typedef struct normData
{
    float max;
    float min;
	float normScalar;
} t_normData;

typedef struct attributeData
{
	float inputData;
	t_normData normData;
	float weight;
	t_attributeIdx order;
	std::string name;
} t_attributeData;


/* ---------------- conversion functions ---------------------- */
float freq2bin(float freq, float n, float sr);
float bin2freq(float bin, float n, float sr);
float freq2bark(float freq);
float bark2freq(float bark);
float freq2mel(float freq);
float mel2freq(float mel);
/* ---------------- END conversion functions ---------------------- */

/* ---------------- utility functions ---------------------- */
void linspace(std::vector<float> &ramp, float start, float finish);
signed char signum(float input);
float euclidDist(t_attributeIdx n, const std::vector<float>& v1, const std::vector<float>& v2, const std::vector<float>& weights, bool sqroot);
float taxiDist(t_attributeIdx n, const std::vector<float>& v1, const std::vector<float>& v2, const std::vector<float>& weights);
float corr(t_attributeIdx n, const std::vector<float>& v1, const std::vector<float>& v2);
void sortKnnInfo(unsigned short int k, t_instanceIdx numInstances, t_instanceIdx prevMatch, std::vector<t_instance>& instances);
/* ---------------- END utility functions ---------------------- */


/* ---------------- filterbank functions ---------------------- */
t_binIdx nearestBinIndex(float target, const float *binFreqs, t_binIdx n);
t_binIdx nearestBinIndex(float target, const std::vector<float> &binFreqs, t_binIdx n);
t_filterIdx getBarkBoundFreqs(std::vector<float> &filterFreqs, float spacing, float sr);
t_filterIdx getMelBoundFreqs(std::vector<float> &filterFreqs, float spacing, float sr);
void createFilterbank(const std::vector<float> &filterFreqs, std::vector<t_filter> &filterbank, t_filterIdx newNumFilters, float window, float sr);
/*  In the next 2 functions filterBank is not const in order to use the tmpValue field of the filters to avoid local memory allocation */
void specFilterBands(t_binIdx n, t_filterIdx numFilters, float *spectrum, std::vector<t_filter> &filterbank, bool normalize);
void filterbankMultiply(float *spectrum, bool normalize, bool filterAvg, std::vector<t_filter> &filterbank, t_filterIdx numFilters);
// void tIDLib_cosineTransform(float *output, t_sample *input, t_filterIdx numFilters);
/* ---------------- END filterbank functions ---------------------- */


/* ---------------- stat computation functions ---------------------- */
/* ---------------- END stat computation functions ---------------------- */


/* ---------------- windowing buffer functions ---------------------- */
void initBlackmanWindow(std::vector<float> &window);
void initCosineWindow(std::vector<float> &window);
void initHammingWindow(std::vector<float> &window);
void initHannWindow(std::vector<float> &window);
/* ---------------- END windowing buffer functions ---------------------- */


/* ---------------- dsp utility functions ---------------------- */

// float tIDLib_ampDB(t_sampIdx n, t_sample *input);
void peakSample(std::vector<float> &input, unsigned long int *peakIdx, float *peakVal);
unsigned long int findAttackStartSamp(std::vector<float> &input, float sampDeltaThresh, unsigned short int numSampsThresh);
unsigned int zeroCrossingRate(const std::vector<float>& buffer);
// void tIDLib_getPitchBinRanges(t_binIdx *binRanges, float thisPitch, float loFreq, t_uChar octaveLimit, float pitchTolerance, t_sampIdx n, float sr);
void power(t_binIdx n, void *fftw_out, float *powBuf);
void mag(t_binIdx n, float *input);
// void tIDLib_normal(t_binIdx n, float *input);
// void tIDLib_normalPeak(t_binIdx n, float *input);
void veclog(t_binIdx n, float *input);
void veclog(t_binIdx n, std::vector<float> &input);
/* ---------------- END dsp utility functions ---------------------- */


// DCT-II
// Same as tIDLib_cosineTransform in original TimbreID (only with constant in pointer)
void unoptimized_discreteCosineTransform(float *output, const float *input, int numFilters)
{
    float piOverNfilters = M_PI/numFilters; // save multiple divides below
    for(int i=0; i<numFilters; i++)
    {
        output[i] = 0;
        for(int k=0; k<numFilters; k++)
            output[i] += input[k] * cos(i * (k+0.5) * piOverNfilters);
    }
}

void unoptimized_discreteCosineTransform(std::vector<float> &output, const std::vector<float> &input, int numFilters)
{
    unoptimized_discreteCosineTransform(&(output[0]), &input[0], numFilters);
}


}
