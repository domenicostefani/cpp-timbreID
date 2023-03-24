/*

Cepstrum - header between Juce and the TimbreID cepstrum~ module
- Originally part of timbreID (Pd library module) by William Brent
- Ported for JUCE usage by Domenico Stefani, 21th April 2020
  (domenico.stefani96@gmail.com)

Porting from cepstrum~.c:
 -> https://github.com/wbrent/timbreID/blob/master/src/cepstrum~.c/

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
class Cepstrum
{
public:

    /** Creates a Cepstrum module with default parameters. */
    Cepstrum();

    /**
     * Cepstrum constructor
     * Creates a Cepstrum module specifying an additional parameter
     * @param analysisWindowSize size of the analysis window
    */
    Cepstrum(unsigned long int analysisWindowSize);

    /** Creates a copy of another Cepstrum module. */
    Cepstrum (const Cepstrum&) = default;

    /** Move constructor */
    Cepstrum (Cepstrum&&) = default;

    /** Free memory used */
    ~Cepstrum();

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
     * Buffer the content of the input audio block
     * @param input audio block to store
     * @param n size of the block
    */
    void storeAudioBlock(const SampleType* input, size_t n) noexcept;

    /**
     * Compute the cepstrum coefficients
     * @return cepstrum coefficients
    */
    std::vector<float>& compute();

    /**
     * Set the window function used
     * Sets the window function (options in tIDLib header file)
     * @param func window fuction used
    */
    void setWindowFunction(tIDLib::WindowFunctionType func) noexcept;
    /**
     * Set the spectrum type used
     * Set whether the power spectrum or the magnitude spectrum is used
     * @param spec spectrum type
    */
    void setSpectrumType(tIDLib::SpectrumType spec) noexcept;

    /**
     * Set the cepstrum type reported
     * Set whether the power cepstrum or the magnitude cepstrum is reported
     * @param ceps cepstrum type
    */
    void setCepstrumType(tIDLib::CepstrumType ceps) noexcept;

    /**
     * Set the SpectrumOffset state
     * Set wehther the spectrum offset is ON or OFF
     * @param offset SpectrumOffset state
    */
    void setSpectrumOffset(bool offset);

    /**
     * Set analysis window size
     * Set a window size value for the analysis. Note that this can be also done
     * from the constructor if there is no need for a change in this value.
     * Window size is not required to be a power of two.
     * @param windowSize the size of the window
    */
    void setWindowSize(uint32_t windowSize);

    /**
     * Get the analysis window size (in samples)
     * @return analysis window size
    */
    unsigned long int getWindowSize() const noexcept;

    /**
     * Return a string containing the main parameters of the module.
     * Refer to the PD helper files of the original timbreID library to know more:
     * https://github.com/wbrent/timbreID/tree/master/help
     * @return string with parameter info
    */
    std::string getInfoString() const noexcept;

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
    tIDLib::CepstrumType cepstrumTypeUsed;   // replaces x_powerCepstrum
    bool spectrumOffset;

   #if ASYNC_FEATURE_EXTRACTION
    uint32_t lastStoreTime; // replaces x_lastDspTime
   #endif

    std::vector<SampleType> signalBuffer;

    std::vector<float> fftwInputVector;
    fftwf_complex *fftwOut;
    fftwf_plan fftwForwardPlan;
    fftwf_plan fftwBackwardPlan;

    std::vector<float> blackman;
    std::vector<float> cosine;
    std::vector<float> hamming;
    std::vector<float> hann;

    std::vector<float> listOut;
};

} // namespace tid
