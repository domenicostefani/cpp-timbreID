/*
  ==============================================================================

  WINDOWED Feature Extractors
  Header that bungles a bunch of feature extractors from the TimbreID Puredata Externals

  Originally programmed by WBrent (https://github.com/wbrent/timbreIDLib)
  Ported to C++/JUCE by Domenico Stefani

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 11 January 2023

  ==============================================================================
*/

#pragma once

// TODO: remove if not needed
#include "juce_timbreID.h"
#include <JuceHeader.h>

#include <regex>

namespace SCL
{
// Class for Scalers and normalizers
struct Scaler
{
    Scaler()
    {
    }
    virtual ~Scaler()
    {
    }
    virtual void scaleFeatureVector(float to_scale[], size_t n) = 0;
    virtual void scaleFeatureVector(std::vector<float> &to_scale) = 0;
};

class MinMaxScaler : public virtual Scaler
{
  public:
    std::vector<float> original_feature_minimums;
    std::vector<float>
        feature_scale; // This is 1/(orig_max-orig_min). So scaler funciton would be (x - orig_min)/(orig_max-orig_min),
                       // but in this case just (x - orig_min)*scale. Multiplication should be faster than division

    MinMaxScaler(std::vector<float> orig_min, std::vector<float> scale) : Scaler()
    {
        original_feature_minimums = orig_min;
        feature_scale = scale;
    }

    ~MinMaxScaler()
    {
    }

    void scaleFeatureVector(float to_scale[], size_t n)
    {
        for (size_t idx = 0; idx < n; ++idx)
            to_scale[idx] = (to_scale[idx] - this->original_feature_minimums[idx]) * this->feature_scale[idx];
    }

    void scaleFeatureVector(std::vector<float> &to_scale)
    {
        scaleFeatureVector(to_scale.data(), to_scale.size());
    }
};

class StandardScaler : public virtual Scaler
{
  public:
    std::vector<float> means;
    std::vector<float> one_over_stds;

    StandardScaler(std::vector<float> means, std::vector<float> stds) : Scaler()
    {
        this->means = means;
        this->one_over_stds = stds;
        for (auto &val : this->one_over_stds)
            val = 1.0 / val;
    }

    ~StandardScaler()
    {
    }

    void scaleFeatureVector(float to_scale[], size_t n)
    {
        for (size_t idx = 0; idx < n; ++idx)
            to_scale[idx] = (to_scale[idx] - this->means[idx]) * this->one_over_stds[idx];
    }

    void scaleFeatureVector(std::vector<float> &to_scale)
    {
        scaleFeatureVector(to_scale.data(), to_scale.size());
    }
};
} // namespace SCL

namespace WFE
{

enum Extractor
{
    ATTACKTIME,
    BARKSPECBRIGHTNESS,
    BARKSPEC,
    BFCC,
    CEPSTRUM,
    MFCC,
    PEAKSAMPLE,
    ZEROCROSSING
};

/**
 * @brief Circular buffer for float vectors, MEANT FOR SINGLE PRODUCER SINGLE CONSUMER USAGE, no concurrency
 *
 * @tparam BUFFER_SIZE
 * @tparam INNER_VEC_SIZE
 */
template <size_t BUFFER_SIZE, size_t INNER_VEC_SIZE> class CircularVectorBuffer
{
  private:
    std::array<std::array<float, INNER_VEC_SIZE>, BUFFER_SIZE> feature_vectors_buffer;
    size_t write_index;

  public:
    CircularVectorBuffer() : write_index(0)
    {
    }

    // getWritePointer method, returns a pointer to C float vector to write
    float *getWritePointer()
    {
        return feature_vectors_buffer[write_index].data();
    }
    void confirmWrite()
    {
        write_index = (write_index + 1) % BUFFER_SIZE;
    }
    const float *at(size_t index)
    {
        // Here index 0 refers to the oldest vector written
        // Which at full buffer is the one at write_index
        return feature_vectors_buffer[(write_index + index) % BUFFER_SIZE].data();
    }
};

/**
 * @brief Class for feature extractors with windowed analysis
 *
 * The whole System collects feature vectors over a window of "Roughly" FEATUREEXT_WINDOW_SIZE samples
 * In practice it collects feature vectors from a set of smaller windows of size FRAME_SIZE audioblocks
 * It applies Hann windowing to the frames
 * The overlap is determined by the FRAME_SIZE and the FRAME_INTERVAL
 * Both FRAME_SIZE and FRAME_INTERVAL are defined in number of audio blocks and not samples
 * The computation includes the first windows that bleed out of the window of interest in the past (including past
 * knowledge) and the last windows that bleed out of the window of interest by adding zeropadding. The number of
 * additional blocks to pad is determined by ZEROPADS
 *
 *
 * Warning: the zeropadding effectively leaves the buffer dirty, but we assume that enough time passes between
 * consecutive calls to compute, that the zeros in the buffer are overwritten by new samples.
 *
 * Eg.
 *  Window of 768 samples (12 blocks of 64 samples)
 *  Frame size of 4 blocks (256 samples)
 *  Frame interval of 2 blocks (128 samples) (meaning that overlap is 50%)
 *  Zeropads of 2 blocks (128 samples)
 *
 *              1
 *    01234567890123456
 *   |------------|
 *  ++++          |   Frame 0 (two blocks in the past and first two blocks of actual window)
 *    ++++        |   Frame 1
 *      ++++      |   Frame 2
 *        ++++    |   Frame 3
 *          ++++  |   Frame 4
 *            ++++|   Frame 5
 *              ++++  Frame 6 (two last blocks of window plus two zero blocks of padding)
 *    01234567890123456
 *
 * @details  This class is a bungled collection of feature extractors from the TimbreID Puredata Externals
 *
 */
template <short FEATUREEXT_WINDOW_SIZE, bool USE_ATTACKTIME, bool USE_BARKSPECBRIGHTNESS, bool USE_BARKSPEC,
          bool USE_BFCC, bool USE_CEPSTRUM, bool USE_MFCC, bool USE_PEAKSAMPLE, bool USE_ZEROCROSSING,
          size_t BLOCK_SIZE, size_t FRAME_SIZE, size_t FRAME_INTERVAL, size_t ZEROPADS>
class FeatureExtractors
{
  private:
    //==================== FEATURE EXTRACTION PARAMS ===========================
    const float BARK_SPACING = 0.5f;
    const float BARK_BOUNDARY = 8.5f;
    const float MEL_SPACING = 100;

    //==================== FEATURE EXTRACTION OBJECTS ==========================

    /*
            Here all the feature extractors have window size equal to FRAME_SIZE * BLOCK_SIZE instead of
       FEATUREEXT_WINDOW_SIZE
    */
    tid::Bfcc<float> bfcc{FRAME_SIZE * BLOCK_SIZE, BARK_SPACING};
    tid::Cepstrum<float> cepstrum{FRAME_SIZE * BLOCK_SIZE};
    tid::AttackTime<float> attackTime{FRAME_SIZE * BLOCK_SIZE};
    tid::BarkSpecBrightness<float> barkSpecBrightness{FRAME_SIZE * BLOCK_SIZE, BARK_SPACING, BARK_BOUNDARY};
    tid::BarkSpec<float> barkSpec{FRAME_SIZE * BLOCK_SIZE, BARK_SPACING};
    tid::Mfcc<float> mfcc{FRAME_SIZE * BLOCK_SIZE, MEL_SPACING};
    tid::PeakSample<float> peakSample{FRAME_SIZE * BLOCK_SIZE};
    tid::ZeroCrossing<float> zeroCrossing{FRAME_SIZE * BLOCK_SIZE};

    std::vector<float> barkSpecRes;
    std::vector<float> bfccRes;
    std::vector<float> cepstrumRes;
    std::vector<float> mfccRes;

    static constexpr int _ATTACKTIME_RES_SIZE = 3;
    static constexpr int _BARKSPECBRIGHTNESS_RES_SIZE = 1;
    static constexpr int _BARKSPEC_RES_SIZE = 50;
    static constexpr int _BFCC_RES_SIZE = 50;
    static constexpr int _CEPSTRUM_RES_SIZE =
        (FRAME_SIZE * BLOCK_SIZE) / 2 + 1; // WindowSize/2+1 (It would be 513 with 1025)
    static constexpr int _MFCC_RES_SIZE = 38;
    static constexpr int _PEAKSAMPLE_RES_SIZE = 2;
    static constexpr int _ZEROCROSSING_RES_SIZE = 1;

    static const unsigned int SINGLE_VECTOR_SIZE =
        _ATTACKTIME_RES_SIZE * USE_ATTACKTIME + _BARKSPECBRIGHTNESS_RES_SIZE * USE_BARKSPECBRIGHTNESS +
        _BARKSPEC_RES_SIZE * USE_BARKSPEC + _BFCC_RES_SIZE * USE_BFCC + _CEPSTRUM_RES_SIZE * USE_CEPSTRUM +
        _MFCC_RES_SIZE * USE_MFCC + _PEAKSAMPLE_RES_SIZE * USE_PEAKSAMPLE + _ZEROCROSSING_RES_SIZE * USE_ZEROCROSSING;

    static const unsigned int BUFFERSIZE = (FEATUREEXT_WINDOW_SIZE * 1.0 / BLOCK_SIZE + ZEROPADS);
    static const unsigned int HOWMANYFRAMES_RES = (BUFFERSIZE / FRAME_INTERVAL);
    static const unsigned int WHOLE_FLATRESMATRIX_SIZE = SINGLE_VECTOR_SIZE * HOWMANYFRAMES_RES;

    juce::AudioBuffer<float> zero_block{1, BLOCK_SIZE}; // Block of zeros for convenience

    std::vector<std::string> whole_header;
    CircularVectorBuffer<BUFFERSIZE, SINGLE_VECTOR_SIZE> feature_vectors_buffer;

    std::array<float, WHOLE_FLATRESMATRIX_SIZE>
        tmpflatFeatureMatrix; // Temporary vector to store the flat feature matrix

    void createWholeHeader()
    {
        whole_header.clear();
        for (int i = 0; i < HOWMANYFRAMES_RES; ++i)
        {
            std::vector<std::string> tmp_header = prefixedHeader(std::to_string(i) + "_");
            whole_header.insert(whole_header.end(), tmp_header.begin(), tmp_header.end());
        }
    }

    int findFeatureInHeader(std::string feature)
    {
        for (size_t i = 0; i < whole_header.size(); ++i)
            if (whole_header[i] == feature)
                return i;
        return -1;
    }

    std::vector<std::string> prefixedHeader(std::string prefix)
    {
        std::vector<std::string> tmp_header;
        if (USE_ATTACKTIME)
        {
            const std::vector<std::string> at_header = {"attackTime_peaksamp", "attackTime_attackStartIdx",
                                                        "attackTime_value"};
            tmp_header.insert(tmp_header.end(), at_header.begin(), at_header.end());
        }
        if (USE_BARKSPECBRIGHTNESS)
        {
            tmp_header.push_back("barkSpecBrightness");
        }
        if (USE_BARKSPEC)
        {
            std::vector<std::string> bs_header;
            bs_header.resize(_BARKSPEC_RES_SIZE);
            for (int i = 0; i < _BARKSPEC_RES_SIZE; i++)
            {
                bs_header[i] = "barkSpec_" + std::to_string(i + 1);
            }
            tmp_header.insert(tmp_header.end(), bs_header.begin(), bs_header.end());
        }
        if (USE_BFCC)
        {
            std::vector<std::string> bfcc_header;
            bfcc_header.resize(_BFCC_RES_SIZE);
            for (int i = 0; i < _BFCC_RES_SIZE; i++)
            {
                bfcc_header[i] = "bfcc_" + std::to_string(i + 1);
            }
            tmp_header.insert(tmp_header.end(), bfcc_header.begin(), bfcc_header.end());
        }
        if (USE_CEPSTRUM)
        {
            std::vector<std::string> cepstrum_header;
            cepstrum_header.resize(_CEPSTRUM_RES_SIZE);
            for (int i = 0; i < _CEPSTRUM_RES_SIZE; i++)
            {
                cepstrum_header[i] = "cepstrum_" + std::to_string(i + 1);
            }
            tmp_header.insert(tmp_header.end(), cepstrum_header.begin(), cepstrum_header.end());
        }
        if (USE_MFCC)
        {
            std::vector<std::string> mfcc_header;
            mfcc_header.resize(_MFCC_RES_SIZE);
            for (int i = 0; i < _MFCC_RES_SIZE; i++)
            {
                mfcc_header[i] = "mfcc_" + std::to_string(i + 1);
            }
            tmp_header.insert(tmp_header.end(), mfcc_header.begin(), mfcc_header.end());
        }
        if (USE_PEAKSAMPLE)
        {
            const std::vector<std::string> ps_header = {"peakSample_value", "peakSample_index"};
            tmp_header.insert(tmp_header.end(), ps_header.begin(), ps_header.end());
        }
        if (USE_ZEROCROSSING)
        {
            tmp_header.push_back("zeroCrossing");
        }

        for (std::string &s : tmp_header)
            s = prefix + s;
        return tmp_header;
    }

    void computeSingleFeatureVector(float featureVector[])
    {
        int last = -1;
        int newLast = 0;

        if (USE_ATTACKTIME)
        {
            /*-----------------------------------------/
            | 01 - Attack time                         |
            /-----------------------------------------*/
            unsigned long int peakSampIdx = 0;
            unsigned long int attackStartIdx = 0;
            float attackTimeValue = 0.0f;
            this->attackTime.compute(&peakSampIdx, &attackStartIdx, &attackTimeValue);

            featureVector[0] = (float)peakSampIdx;
            featureVector[1] = (float)attackStartIdx;
            featureVector[2] = attackTimeValue;
            newLast = 2;
#ifdef LOG_SIZES
            info += ("attackTime [" + std::to_string(last + 1) + ", " + std::to_string(newLast) + "]\n");
#endif
            last = newLast;
        }

        if (USE_BARKSPECBRIGHTNESS)
        {
            /*-----------------------------------------/
            | 02 - Bark Spectral Brightness            |
            /-----------------------------------------*/
            float bsb = this->barkSpecBrightness.compute();

            featureVector[3] = bsb;
            newLast = 3;
#ifdef LOG_SIZES
            info += ("barkSpecBrightness [" + std::to_string(last + 1) + ", " + std::to_string(newLast) + "]\n");
#endif
            last = newLast;
        }

        if (USE_BARKSPEC)
        {
            /*-----------------------------------------/
            | 03 - Bark Spectrum                       |
            /-----------------------------------------*/
            barkSpecRes = this->barkSpec.compute();

            jassert(barkSpecRes.size() == _BARKSPEC_RES_SIZE);
            for (int i = 0; i < _BARKSPEC_RES_SIZE; ++i)
            {
                featureVector[(last + 1) + i] = barkSpecRes[i];
            }
            newLast = last + _BARKSPEC_RES_SIZE;
#ifdef LOG_SIZES
            info += ("barkSpec [" + std::to_string(last + 1) + ", " + std::to_string(newLast) + "]\n");
#endif
            last = newLast;
        }

        if (USE_BFCC)
        {
            /*------------------------------------------/
            | 04 - Bark Frequency Cepstral Coefficients |
            /------------------------------------------*/
            bfccRes = this->bfcc.compute();
            jassert(bfccRes.size() == _BFCC_RES_SIZE);
            for (int i = 0; i < _BFCC_RES_SIZE; ++i)
            {
                featureVector[(last + 1) + i] = bfccRes[i];
            }
            newLast = last + _BFCC_RES_SIZE;
#ifdef LOG_SIZES
            info += ("bfcc [" + std::to_string(last + 1) + ", " + std::to_string(newLast) + "]\n");
#endif
            last = newLast;
        }

        if (USE_CEPSTRUM)
        {
            /*------------------------------------------/
            | 05 - Cepstrum Coefficients                |
            /------------------------------------------*/
            cepstrumRes = this->cepstrum.compute();
            jassert(cepstrumRes.size() == _CEPSTRUM_RES_SIZE);
            for (int i = 0; i < _CEPSTRUM_RES_SIZE; ++i)
            {
                featureVector[(last + 1) + i] = cepstrumRes[i];
            }
            newLast = last + _CEPSTRUM_RES_SIZE;
#ifdef LOG_SIZES
            info += ("cepstrum [" + std::to_string(last + 1) + ", " + std::to_string(newLast) + "]\n");
#endif
            last = newLast;
        }

        if (USE_MFCC)
        {
            /*-----------------------------------------/
            | 06 - Mel Frequency Cepstral Coefficients |
            /-----------------------------------------*/
            mfccRes = this->mfcc.compute();
            jassert(mfccRes.size() == _MFCC_RES_SIZE);
            for (int i = 0; i < _MFCC_RES_SIZE; ++i)
            {
                featureVector[(last + 1) + i] = mfccRes[i];
            }
            newLast = last + _MFCC_RES_SIZE;
#ifdef LOG_SIZES
            info += ("mfcc [" + std::to_string(last + 1) + ", " + std::to_string(newLast) + "]\n");
#endif
            last = newLast;
        }

        if (USE_PEAKSAMPLE)
        {
            /*-----------------------------------------/
            | 07 - Peak sample                         |
            /-----------------------------------------*/
            std::pair<float, unsigned long int> peakSample = this->peakSample.compute();
            float peakSampleRes = peakSample.first;
            unsigned long int peakSampleIndex = peakSample.second;
            featureVector[last + 1] = peakSampleRes;
            featureVector[last + 2] = peakSampleIndex;
            newLast = last + 2;
#ifdef LOG_SIZES
            info += ("peakSample [" + std::to_string(last + 1) + ", " + std::to_string(newLast) + "]\n");
#endif
            last = newLast;
        }

        if (USE_ZEROCROSSING)
        {
            /*-----------------------------------------/
            | 08 - Zero Crossings                      |
            /-----------------------------------------*/
            uint32 crossings = this->zeroCrossing.compute();
            featureVector[last + 1] = crossings;
            newLast = last + 1;
#ifdef LOG_SIZES
            info += ("zeroCrossing [" + std::to_string(last + 1) + ", " + std::to_string(newLast) + "]\n");
#endif
            last = newLast;
        }
    }

  public:
    std::unique_ptr<SCL::Scaler> scaler;

    /**
     * @brief Filter to output only a subset of the features that was selected during training
     * This filter is used to reduce the number of features in output, which are fed to the classifier.
     * During classifier training, feature selection can be used: the subset of selected features can be specified
     * in the configuration file, and this filter will be used to output only the selected features.
     *
     */
    class FeatureFilter
    {
        std::vector<size_t> indexes; // Indexes of the features to keep
        size_t filtered_matrix_size = -1, filtered_matrix_nrows = -1, filtered_matrix_ncols = -1;

      public:
        FeatureFilter(std::vector<size_t> indexes, size_t filtered_matrix_size, size_t filtered_matrix_nrows,
                      size_t filtered_matrix_ncols)
            : indexes(indexes), filtered_matrix_size(filtered_matrix_size),
              filtered_matrix_nrows(filtered_matrix_nrows), filtered_matrix_ncols(filtered_matrix_ncols)
        {
            const bool verbose = false;
            if (verbose)
            {
                std::cout << "FeatureFilter: " << indexes.size() << " features selected" << std::endl;
                std::cout << "filtered_matrix_size: " << filtered_matrix_size << std::endl;
                std::cout << "filtered_matrix_nrows: " << filtered_matrix_nrows << std::endl;
                std::cout << "filtered_matrix_ncols: " << filtered_matrix_ncols << std::endl << std::flush;
            }
        }

        std::vector<float> filterOffline(const std::vector<float> &features)
        {
            std::vector<float> filteredFeatures;
            for (const auto &index : indexes)
            {
                filteredFeatures.push_back(features[index]);
            }
            return filteredFeatures;
        }

        size_t getFilteredMatrixSize() const
        {
            return indexes.size();
        }

        size_t getFilteredMatrixNRows() const
        {
            return filtered_matrix_nrows;
        }

        size_t getFilteredMatrixNCols() const
        {
            return filtered_matrix_ncols;
        }

        // void filterRealtime(const std::vector<float>& features_in, std::vector<float>& features_out) {
        //     if (features_out.size() != indexes.size())
        //         throw std::runtime_error("FeatureFilter::filterRealtime: features_out size mismatch");
        //     if (features_in.size() != features_out.size())
        //         throw std::runtime_error("FeatureFilter::filterRealtime: features_in size mismatch");
        //     features_out.clear();
        //     for (const auto& index : indexes) {
        //         features_out.push_back(features_in[index]);
        //     }
        // }
        void filterRealtime(const float features_in[], size_t in_size, float features_out[], size_t out_size)
        {
            if (out_size != indexes.size())
                throw std::runtime_error("FeatureFilter::filterRealtime: features_out size mismatch");
            if (in_size <= out_size)
                throw std::runtime_error("Output should have a smaller size than input");
            for (int idx = 0; idx < out_size; idx++)
            {
                features_out[idx] = features_in[indexes[idx]];
            }
        }
    };

    std::unique_ptr<FeatureFilter> featureFilter;

    /**
     * @brief Set the Feature Selection Filter object
     * When called, this function sets the feature selection filter for the computed features.
     * This is used when feature selection was applied before training, and inference requires a matching
     * selection process.
     * A list of selected features is meant to be copied from the training script or output_folder/info.txt
     * and copied to the json config file at NN_CONFIG_JSON_PATH ("/udata/phase3experiment.json").
     * (REMEMBER to change single quotes to double quotes because of the json format)
     *
     * Here, the list of feature names extracted from the json config is read, and the indexes (relative to
     * the computed flat matrix of features) are stored in the featureFilter object.
     *
     * @param selectedFeatures the list of selected features (from the json config file)
     * @param nrows           returns the number of rows in the computed feature matrix
     * @param ncols           returns the number of columns in the computed feature matrix
     */
    void setFeatureSelectionFilter(const std::vector<std::string> &selectedFeatures, int &nrows, int &ncols,
                                   bool verbose = false)
    {
        const bool VERBOSE_DEBUG = false;
        if (VERBOSE_DEBUG)
        {
            std::cout << "FeatureFilter::setFeatureSelectionFilter" << std::endl;
            size_t count = 0;
            for (const auto &feat : selectedFeatures)
                std::cout << count++ << " " << feat << std::endl;
            std::cout << std::flush;
        }

        std::vector<std::string> computed_feature_names = this->getHeader();

        std::vector<size_t> indexes;
        // Get indexes of the selected feature names in the computed_feature_names vector (using the header)
        for (const auto &selectedFeature : selectedFeatures)
        {
            for (size_t i = 0; i < computed_feature_names.size(); ++i)
            {
                if (computed_feature_names[i] == selectedFeature)
                {
                    indexes.push_back(i);
                }
            }
        }

        // std::cout << "Size of indexesSize of selected: " << indexes.size() << std::endl;
        // std::cout << "Features: " << selectedFeatures.size() << std::endl << std::flush;
        // rtlogger.logValue("Size of indexes", indexes.size());
        // rtlogger.logValue("Size of selected", selectedFeatures.size());

        if (indexes.size() != selectedFeatures.size())
        {
            std::cout << "ERROR: Some selected features were not found in the computed features" << std::endl
                      << std::flush;
            std::cout << "Selected features: " << std::endl << std::flush;
            for (const auto &selectedFeature : selectedFeatures)
            {
                std::cout << selectedFeature << std::endl << std::flush;
            }
            std::cout << "Computed features: " << std::endl << std::flush;
            for (const auto &computedFeature : computed_feature_names)
            {
                std::cout << computedFeature << std::endl << std::flush;
            }
            std::cout << "Indexes: " << std::endl << std::flush;
            for (const auto &index : indexes)
            {
                std::cout << index << std::endl << std::flush;
            }
            throw std::runtime_error("Some selected features were not found in the computed features");
        }

        // save in a constant the size of the filtered flat matrix
        if (verbose)
        {
            std::cout << "Saving in a constant the size of the filtered flat matrix" << std::endl << std::flush;
        }
        size_t filtered_matrix_size = indexes.size();
        if (verbose)
        {
            std::cout << "filtered_matrix_size: " << filtered_matrix_size << std::endl << std::flush;
        }
        // Use regex to find the maximum int prefix eg. "1_*"
        std::regex re("^([0-9]+)_*");
        std::smatch match;
        bool prefix_is_zerobased = false;
        for (const auto &selectedFeature : selectedFeatures)
        {
            if (std::regex_search(selectedFeature, match, re))
            {
                int prefix = std::stoi(match[1]);
                if (prefix > nrows)
                    nrows = (size_t)prefix;
                if (prefix == 0)
                    prefix_is_zerobased = true;
            }
        }

        if (prefix_is_zerobased)
            nrows++;

        if (verbose)
        {
            std::cout << "nrows: " << nrows << std::endl << std::flush;
            std::cout << "filtered_matrix_size % nrows = " << filtered_matrix_size << " % " << nrows << " = "
                      << (filtered_matrix_size % nrows) << std::endl
                      << std::flush;
        }

        // flat matrix length and cols must be divisible
        if (filtered_matrix_size % nrows != 0)
        {
            throw std::runtime_error("FeatureFilter::setFeatureSelectionFilter: filtered_matrix_size % nrows != 0");
        }
        ncols = (size_t)(filtered_matrix_size / nrows);

        if (verbose)
            std::cout << "ncols: " << ncols << std::endl << std::flush;

        this->featureFilter = std::make_unique<FeatureFilter>(indexes, filtered_matrix_size, nrows, ncols);
    }

    void setFeatureScaler(std::unique_ptr<SCL::Scaler> scaler)
    {
        this->scaler = std::move(scaler);
    }

    // Constructor
    FeatureExtractors()
    {
        barkSpecRes.resize(_BARKSPEC_RES_SIZE);
        bfccRes.resize(_BFCC_RES_SIZE);
        cepstrumRes.resize(_CEPSTRUM_RES_SIZE);
        mfccRes.resize(_MFCC_RES_SIZE);

        // Set Hann window as default
        bfcc.setWindowFunction(tIDLib::WindowFunctionType::hann);
        barkSpec.setWindowFunction(tIDLib::WindowFunctionType::hann);
        barkSpecBrightness.setWindowFunction(tIDLib::WindowFunctionType::hann);
        cepstrum.setWindowFunction(tIDLib::WindowFunctionType::hann);
        mfcc.setWindowFunction(tIDLib::WindowFunctionType::hann);
        // attackTime.
        // peakSample.  // These have no window function
        // zeroCrossing.

        attackTime.setMaxSearchRange(20);

        // for (int i = 0; i < zero_block.size(); ++i)
        //     zero_block[i] = .0f;

        // Clear zero_block so that it is filled with zeros
        zero_block.clear();

        getHeader();
    }

    constexpr static size_t getFeVectorSize()
    {
        // The matrix of featurevectors is returned as a flat vector of size rows*cols
        return WHOLE_FLATRESMATRIX_SIZE;
    }

    std::vector<std::string> getHeader()
    {
        if (whole_header.size() == 0)
            createWholeHeader();
        return whole_header;
    }

    void prepare(double sampleRate, unsigned int samplesPerBlock)
    {
        /** Prepare feature extractors **/
        bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
        cepstrum.prepare(sampleRate, (uint32)samplesPerBlock);
        attackTime.prepare(sampleRate, (uint32)samplesPerBlock);
        barkSpecBrightness.prepare(sampleRate, (uint32)samplesPerBlock);
        barkSpec.prepare(sampleRate, (uint32)samplesPerBlock);
        mfcc.prepare(sampleRate, (uint32)samplesPerBlock);
        peakSample.prepare(sampleRate, (uint32)samplesPerBlock);
        zeroCrossing.prepare(sampleRate, (uint32)samplesPerBlock);
    }

    void reset()
    {
        /*------------------------------------/
        | Reset the feature extractors        |
        /------------------------------------*/
        bfcc.reset();
        cepstrum.reset();
        attackTime.reset();
        barkSpecBrightness.reset();
        barkSpec.reset();
        mfcc.reset();
        peakSample.reset();
        zeroCrossing.reset();
    }

    template <typename SampleType> void storeAndCompute(AudioBuffer<SampleType> &buffer, short int channel)
    {
        if (channel < 0 || channel >= buffer.getNumChannels())
            throw std::runtime_error("FeatureExtractors::storeAndCompute: channel out of range, must be in range [0," +
                                     std::to_string(buffer.getNumChannels() - 1) + "]");

        if (USE_BFCC)
            bfcc.store(buffer, channel);
        if (USE_CEPSTRUM)
            cepstrum.store(buffer, channel);
        if (USE_ATTACKTIME)
            attackTime.store(buffer, channel);
        if (USE_BARKSPECBRIGHTNESS)
            barkSpecBrightness.store(buffer, channel);
        if (USE_BARKSPEC)
            barkSpec.store(buffer, channel);
        if (USE_MFCC)
            mfcc.store(buffer, channel);
        if (USE_PEAKSAMPLE)
            peakSample.store(buffer, channel);
        if (USE_ZEROCROSSING)
            zeroCrossing.store(buffer, channel);

        computeSingleFeatureVector(feature_vectors_buffer.getWritePointer());
        feature_vectors_buffer.confirmWrite();
    }

    std::string getInfoString(WFE::Extractor extractor)
    {
        switch (extractor)
        {
        case ATTACKTIME:
            return attackTime.getInfoString();
            break;
        case BARKSPECBRIGHTNESS:
            return barkSpecBrightness.getInfoString();
            break;
        case BARKSPEC:
            return barkSpec.getInfoString();
            break;
        case BFCC:
            return bfcc.getInfoString();
            break;
        case CEPSTRUM:
            return cepstrum.getInfoString();
            break;
        case MFCC:
            return mfcc.getInfoString();
            break;
        case PEAKSAMPLE:
            return peakSample.getInfoString();
            break;
        case ZEROCROSSING:
            return zeroCrossing.getInfoString();
            break;
        default:
            throw std::logic_error("Invalid Extractor");
        }
    }

    void computeFeatureVectors(float flatFeatureMatrix[])
    {
        // Here we do not really compute all the feature vectors, but we do the following:
        // 1. We pad the buffer with as many zeroblock as ZEROPADS says. Doing that we compute the feature vectors for
        // only these few times
        // 2. We take the feature vectors from the buffer, one every FRAME_INTERVAL, totaling to HOWMANYFRAMES_RES
        // 3. these are all returned as a flat matrix of size HOWMANYFRAMES_RES * SINGLE_VECTOR_SIZE

        for (int i = 0; i < ZEROPADS; ++i)
            storeAndCompute(zero_block, 0);
        for (int i = 0; i < HOWMANYFRAMES_RES; ++i)
        {
            const size_t index = i * FRAME_INTERVAL + 1;
            // printf("index: %ld\n", index);
            const float *featureVector = feature_vectors_buffer.at(index);
            for (int j = 0; j < SINGLE_VECTOR_SIZE; ++j)
            {
                flatFeatureMatrix[i * SINGLE_VECTOR_SIZE + j] = featureVector[j];
            }
        }
    }

    void computeSelectedFeaturesAndScale(float flatFilteredMatrix[])
    {
        // Here we first call computeFeatureVectors, then we throw away the features that are not in the "selected" list
        // At last, we scale the features using the scaler
        const size_t filtered_flatmatrix_size = this->featureFilter->getFilteredMatrixSize();
        if (WHOLE_FLATRESMATRIX_SIZE != this->tmpflatFeatureMatrix.size())
            throw std::logic_error("WHOLE_FLATRESMATRIX_SIZE does not match the size of the temporary matrix (" +
                                   std::to_string(WHOLE_FLATRESMATRIX_SIZE) +
                                   " != " + std::to_string(this->tmpflatFeatureMatrix.size()) + ")");

        computeFeatureVectors(this->tmpflatFeatureMatrix.data());

        // Filtering (applying feature selection, not computing the actual best selection, only applying it)
        if (this->featureFilter != nullptr)
            this->featureFilter->filterRealtime(this->tmpflatFeatureMatrix.data(), this->tmpflatFeatureMatrix.size(),
                                                flatFilteredMatrix, filtered_flatmatrix_size);
        else
            throw std::logic_error("Feature filter was NOT set with setFeatureSelectionFilter");

        // Scaling
        if (this->scaler != nullptr)
        {
            if (SCL::MinMaxScaler *sclPtr = dynamic_cast<SCL::MinMaxScaler *>(this->scaler.get()))
                sclPtr->scaleFeatureVector(flatFilteredMatrix, filtered_flatmatrix_size);
            else if (SCL::StandardScaler *sclPtr = dynamic_cast<SCL::StandardScaler *>(this->scaler.get()))
                sclPtr->scaleFeatureVector(flatFilteredMatrix, filtered_flatmatrix_size);
            else
                throw std::logic_error("Unknown scaler type");
        }
    }
};

} // namespace WFE
