#pragma once

#include <vector>
#include "fftw3.h"
#include <cmath>
#include <string>
#include <stdexcept>

typedef unsigned long int t_binIdx; // 0 to 18,446,744,073,709,551,615
typedef unsigned short int t_filterIdx;
typedef unsigned int t_attributeIdx; // gets us over 4 billion attributes. should be plenty indices to instances in database. members of a cluster. non-negative only
typedef unsigned int t_instanceIdx;

#define TIDVERSION "0.8.2C"

/** ASYNC_FEATURE_EXTRACTION
 * If true, this enables the computation of the time interval between the last
 * buffering operation and  any feature exraction method call.
 * This allows each module to compute where the call to the compute metods falls,
 * inside the current audio block (in samples)
 * 
 * This value in samples is used to offset the beginning of the signal buffer 
 * slice on which the features are computed.
 * The final computaton slice obtained finishes exacly one audio_block_size before
 * the current time, which causes the beginning to be always at the same time in 
 * relation to the time of computation.
 * 
 * This should remain disabled (FALSE) for the following reasons:
 *  - Modules currently use tid::Time::currentTimeMillis() which IS NOT A STEADY CLOCK (https://forum.juce.com/t/time-currenttimemillis-goes-back-in-time-sometimes/16440/6)
 *    Time::getMillisecondCounter() or Time::getMillisecondCounterHiRes() should be used instead (AT LEAST, check next bullet point).
 *  - IT IS ONLY FOR REAL TIME : If it is needed to run the modules quicker than real time (render in DAW), even JUCE's steady timers 
 *                               should be avoided since time intervals lose their relationship with samples in offline contexts.
 *  - Small block sizes and high samplerates reduce the relevance of this nuance (64samples at 48kHz is ~1.33ms), rendering this less useful.
 *  * More: tid::Time::currentTimeMillis() causes catastrophic failure on the real-time OS Xenomai (Elk Audio OS).
 */
#define ASYNC_FEATURE_EXTRACTION false

#if !ASYNC_FEATURE_EXTRACTION
/**
 * MANUAL SYNC OFFSET for FEATURE EXTRACTION VECTOR
 * If the block offset computation for precise computation interval is disabled,
 * here the fixed offset can be specified.
 * 
 * |<------------------- Signal buffer ------------------->|
 * 
 *                                                      /----> audio_block size (This is the last block stored)
 *  <------------- analyis window size -------------> <-*->    
 * |-------------------------------------------------|-----|  *  |
 *                                                            \--> Here computation is called (somewhere inside the new audio block)
 * 
 * Different feature computation vectors used, depending on manual offset:
 * 
 * |<------ computation buff. with offset 0.0 ------>|
 *       |<------ computation buff. with offset 1.0 ------>|
 *    |<------ computation buff. with offset 0.5 ------>|
*/
namespace tIDLib
{
    static const double FEATURE_EXTRACTION_OFFSET = 1.0;
}
#endif

// choose either FFTW_MEASURE or FFTW_ESTIMATE here.
#define FFTWPLANNERFLAG (FFTW_ESTIMATE | FFTW_CONSERVE_MEMORY)

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

namespace tIDLib
{
constexpr unsigned int BLOCKSIZEDEFAULT = 64;
constexpr float MINBARKSPACING = 0.1f;
constexpr float MAXBARKSPACING = 6.0f;
constexpr float BARKSPACINGDEFAULT = 0.5f;
constexpr float MINMELSPACING = 5.0f;
constexpr float MAXMELSPACING = 1000.0f;
constexpr float MELSPACINGDEFAULT = 100.0f;
constexpr unsigned long int WINDOWSIZEDEFAULT = 1024;
constexpr unsigned long int MINWINDOWSIZE = 4;
constexpr unsigned long int  SAMPLERATEDEFAULT = 44100;
constexpr float MAXBARKS = 26.0f;
constexpr float MAXBARKFREQ = 22855.4f;
constexpr float MAXMELFREQ = 22843.6f;
constexpr float MAXMELS = 3962.0f;
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


/** DCT-II Computation without any optimization
 * Its the same function as the original tIDLib_cosineTransform of TimbreID
 */
inline void unoptimized_discreteCosineTransform(float *output, const float *input, int numFilters)
{
    float piOverNfilters = M_PI/numFilters; // save multiple divides below
    for(int i=0; i<numFilters; i++)
    {
        output[i] = 0.0f;
        for(int k=0; k<numFilters; k++)
            output[i] += input[k] * cos(i * (k+0.5f) * piOverNfilters);
    }
}

/** DCT-II Computation without any optimization
 * Its the same function as the original tIDLib_cosineTransform of TimbreID.
 * This wrapper accepts std::vectors
 */
inline void unoptimized_discreteCosineTransform(std::vector<float> &output, const std::vector<float> &input, int numFilters)
{
    unoptimized_discreteCosineTransform(&(output[0]), &input[0], numFilters);
}

/** DCT-II Computation module with optimized precomputation
 * This module can precompute the Basis for the dct transform of a specific size
 * precomputeBasis() allocates memory, while compute()
*/
template <typename FloatType>
class DiscreteCosineTransform
{
public:
    DiscreteCosineTransform(){}
    ~DiscreteCosineTransform(){}

    /** Precompute DCT basis to optimize computation
     * Do not call this function from a real-time thread.
    */
    void precomputeBasis(int transformSize)
    {
        if (transformSize < 1)
            throw std::logic_error("DCT size has to be >= 1");
        basis.resize(transformSize);

        float piOverNfilters = M_PI/transformSize;
        for(int i=0; i<transformSize; ++i)
            for(int k=0; k<transformSize; ++k)
                basis.at(i,k) = cos(i * (k+0.5) * piOverNfilters);
    }

    /** Compute the dct transform (DCT-II)
     * This is optimized and safe to be called from a real-time thread
    */
    void compute(const std::vector<FloatType>& input, std::vector<FloatType>& output)
    {
        if (output.size() != basis.size())
            throw std::logic_error("Output vector size must match the size of the basis");
        if (input.size() != basis.size())
            throw std::logic_error("Input vector size must match the size of the basis");

        compute(input,output,output.size());
    }


    /** Compute the dct transform (DCT-II)
     * This is optimized and safe to be called from a real-time thread
     * In case that only a portion of the vectors has to be considered,
     * transformSize determines how many elements to consider.
     * It should still match the size specified during precomputation
    */
    void compute(const std::vector<FloatType>& input, std::vector<FloatType>& output, size_t transformSize)
    {
        if (transformSize != basis.size())
            throw std::logic_error("transformSize vector size must match the size of the basis");

        for(size_t i=0; i<transformSize; i++)
        {
            output[i] = 0;
            for(size_t k=0; k<transformSize; k++)
                output[i] += input[k] * basis.at(i,k);
        }
    }
private:
    /** Matrix for the dct basis */
    class Basis
    {
    public:
        /**
         * Resize the matrix, allowing to fill the cells and use them without
         * unexpected reallocation
        */
        void resize(size_t newSize)
        {
            values.resize(newSize);
            for(std::vector<FloatType>& subvec : values)
                subvec.resize(newSize);
        }
        /** Access a cell of the basis matrix */
        FloatType& at(size_t i, size_t k) { return values[i][k]; }
        /** Return the size of the square matrix */
        size_t size() { return values.size(); }
    private:
        std::vector<std::vector<FloatType>> values;
    };

    Basis basis; // basis of the DCT transform
};


}
