#pragma once


#include "tIDLib.h"
#include "fftw3.h"
#include <stdexcept>
#include <cassert>

namespace tid
{

template <typename SampleType>
class Bfcc
{
public:

    /** Creates a Bfcc module with default parameters. */
    Bfcc();

    /**
     * Bfcc constructor
     * Creates a Bfcc module specifying an additional parameter
     * @param analysisWindowSize size of the analysis window
    */
    Bfcc(unsigned long int analysisWindowSize);

    /**
     * Bfcc constructor
     * Creates a Bfcc module specifying additional parameters
     * @param analysisWindowSize size of the analysis window
     * @param barkSpacing filter spacing in Barks
    */
    Bfcc(unsigned long int analysisWindowSize, float barkSpacing);
    /** Creates a copy of another Bfcc module. */
    Bfcc (const Bfcc&) = default;

    /** Move constructor */
    Bfcc (Bfcc&&) = default;

    /** Free memory used */
    ~Bfcc();

    //==========================================================================

    /**
     * Initialization of the module
     * Prepares the module to play by clearing buffers and setting audio buffers
     * standard parameters
     * @param sampleRate The sample rate of the buffer
     * @param blockSize The size of the individual audio blocks
    */
    void prepare (unsigned long int sampleRate, unsigned int blockSize) noexcept;

    /**
     * Resets the processing pipeline.
     * It empties the analysis buffer
    */
    void reset() noexcept;

    /**
     * Construct a new filterbank with a specific spacing.
     * @param barkSpacing spacing for the new filterbank (in Barks)
    */
    void createFilterbank(float barkSpacing);

    /**
     * Buffer the content of the input audio block
     * @param input audio block to store
     * @param n size of the block
    */
    void storeAudioBlock(const SampleType* input, size_t n) noexcept;

    /**
     * Compute the bark frequency cepstral coefficients
     * @return cepstral coefficients
    */
    std::vector<float>& compute();
    /*--------------------------- Setters/getters ----------------------------*/

    /**
     * Set the window function used
     * Sets the window function (options in tIDLib header file)
     * @param func window fuction used
    */
    void setWindowFunction(tIDLib::WindowFunctionType func) noexcept;

    /**
     * Set the state of the filter
     * Set whether the triangular Bark spaced filters are used (filterEnabled)
     * OR energy is averaged in the in the unfiltered bins(filterDisabled).
     * (With filterEnabled, sum OR avg are performed depending on which
     * operation is specified with the Bark::setFilterOperation method)
     * (From wbrent help examples)
     * @param state filter state to set
    */
    void setFiltering(tIDLib::FilterState state) noexcept;

    /**
     * Set the operation to perform on the energy in each filter
     * If using the triangular Bark spaced filters, you can either sum or
     * average the energy in each filter .(default: sum)
     * (IF FILTER WAS ENABLED with the setFiltering method)
     * (From wbrent help examples)
     * @param operationType type of operation selected
    */
    void setFilterOperation(tIDLib::FilterOperation operationType) noexcept;

    /**
     * Set the spectrum type used
     * Set whether the power spectrum or the magnitude spectrum is used
     * @param spec spectrum type
    */
    void setSpectrumType(tIDLib::SpectrumType spec) noexcept;

    /**
     * Set analysis window size
     * Set a window size value for the analysis. Note that this can be also done
     * from the constructor if there is no need for a change in this value.
     * Window size is not required to be a power of two.
     * @param windowSize the size of the window
    */
    void setWindowSize(uint32_t windowSize);

    /**
     *  Set the normalization flag
     * @param norm normalization flag value
    */
    void setNormalize(bool norm) noexcept;

    /**
     * Get the analysis window size (in samples)
     * @return analysis window size
    */
    unsigned long int getWindowSize() const noexcept;

    /**
     * Get the normalize flag value
     * @return analysis window size
    */
    bool getNormalize() const noexcept;

    /**
     * Return a string containing the main parameters of the module.
     * Refer to the PD helper files of the original timbreID library to know more:
     * https://github.com/wbrent/timbreID/tree/master/help
     * @return string with parameter info
    */
    std::string getInfoString() const;

private:

    /**
     * Initialize the parameters of the module.
    */
    void initModule();

    /**
     *  Free memory allocated
    */
    void freeMem();

    //==========================================================================

    float sampleRate;
    float blockSize;

    unsigned long int analysisWindowSize;

    tIDLib::WindowFunctionType windowFunction;
    tIDLib::SpectrumType spectrumTypeUsed;   // replaces x_powerSpectrum

   #if ASYNC_FEATURE_EXTRACTION
    uint32 lastStoreTime; // replaces x_lastDspTime
   #endif

    std::vector<SampleType> signalBuffer;

    std::vector<float> fftwInputVector;
    fftwf_complex *fftwOut;
    fftwf_plan fftwPlan;
    tIDLib::DiscreteCosineTransform<SampleType> dctPlan;

    std::vector<float> blackman;
    std::vector<float> cosine;
    std::vector<float> hamming;
    std::vector<float> hann;

    t_filterIdx sizeFilterFreqs;
    t_filterIdx numFilters;

    float barkSpacing;
    std::vector<float> filterFreqs;
    std::vector<tIDLib::t_filter> filterbank;

    tIDLib::FilterState filterState;         // replaces x_specBandAvg
    tIDLib::FilterOperation filterOperation; // replaces x_filterAvg
    bool normalize;

    std::vector<float> coefficientsVector;
};

}