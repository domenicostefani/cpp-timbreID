/*
  ==============================================================================

  Feature Extractors
  Header that bungles a bunch of feature extractors from the TimbreID Puredata Externals

  Originally programmed by WBrent (https://github.com/wbrent/timbreIDLib)
  Ported to C++/JUCE by Domenico Stefani

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 11 January 2023

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "juce_timbreID.h"
#include <cassert>


namespace FE {

enum Extractor{ATTACKTIME,BARKSPECBRIGHTNESS,BARKSPEC,BFCC,CEPSTRUM,MFCC,PEAKSAMPLE,ZEROCROSSING};


/**
 * @brief Class for feature extractors.
 * @details  This class is a bungled collection of feature extractors from the TimbreID Puredata Externals
 * 
 */
template<short FEATUREEXT_WINDOW_SIZE,
          bool USE_ATTACKTIME,
          bool USE_BARKSPECBRIGHTNESS,
          bool USE_BARKSPEC,
          bool USE_BFCC,
          bool USE_CEPSTRUM,
          bool USE_MFCC,
          bool USE_PEAKSAMPLE,
          bool USE_ZEROCROSSING>
class FeatureExtractors {
private:
    //==================== FEATURE EXTRACTION PARAMS ===========================
    const float BARK_SPACING = 0.5f;
    const float BARK_BOUNDARY = 8.5f;
    const float MEL_SPACING = 100;

    //==================== FEATURE EXTRACTION OBJECTS ==========================
    tid::Bfcc<float> bfcc{FEATUREEXT_WINDOW_SIZE, BARK_SPACING};
    tid::Cepstrum<float> cepstrum{FEATUREEXT_WINDOW_SIZE};
    tid::AttackTime<float> attackTime{FEATUREEXT_WINDOW_SIZE};
    tid::BarkSpecBrightness<float> barkSpecBrightness{FEATUREEXT_WINDOW_SIZE, BARK_SPACING, BARK_BOUNDARY};
    tid::BarkSpec<float> barkSpec{FEATUREEXT_WINDOW_SIZE, BARK_SPACING};
    tid::Mfcc<float> mfcc{FEATUREEXT_WINDOW_SIZE, MEL_SPACING};
    tid::PeakSample<float> peakSample{FEATUREEXT_WINDOW_SIZE};
    tid::ZeroCrossing<float> zeroCrossing{FEATUREEXT_WINDOW_SIZE};

    std::vector<float> barkSpecRes;
    std::vector<float> bfccRes;
    std::vector<float> cepstrumRes;
    std::vector<float> mfccRes;

    static constexpr int _ATTACKTIME_RES_SIZE = 3;
    static constexpr int _BARKSPECBRIGHTNESS_RES_SIZE = 1;
    static constexpr int _BARKSPEC_RES_SIZE = 50;
    static constexpr int _BFCC_RES_SIZE = 50;
    static constexpr int _CEPSTRUM_RES_SIZE = FEATUREEXT_WINDOW_SIZE/2+1; // WindowSize/2+1 (It would be 513 with 1025)
    static constexpr int _MFCC_RES_SIZE = 38;
    static constexpr int _PEAKSAMPLE_RES_SIZE = 2;
    static constexpr int _ZEROCROSSING_RES_SIZE = 1;

    static const unsigned int VECTOR_SIZE = _ATTACKTIME_RES_SIZE*USE_ATTACKTIME+\
                                            _BARKSPECBRIGHTNESS_RES_SIZE*USE_BARKSPECBRIGHTNESS+\
                                            _BARKSPEC_RES_SIZE*USE_BARKSPEC+\
                                            _BFCC_RES_SIZE*USE_BFCC+\
                                            _CEPSTRUM_RES_SIZE*USE_CEPSTRUM+\
                                            _MFCC_RES_SIZE*USE_MFCC+\
                                            _PEAKSAMPLE_RES_SIZE*USE_PEAKSAMPLE+\
                                            _ZEROCROSSING_RES_SIZE*USE_ZEROCROSSING;

    std::vector<std::string> header;

    void createHeader() {
        header.clear();
        if (USE_ATTACKTIME) {
            const std::vector<std::string> at_header = {"attackTime_peaksamp","attackTime_attackStartIdx","attackTime_value"};
            header.insert(header.end(),at_header.begin(),at_header.end());
        }
        if (USE_BARKSPECBRIGHTNESS) {
            header.push_back("barkSpecBrightness");
        }
        if (USE_BARKSPEC) {
            std::vector<std::string> bs_header;
            bs_header.resize(_BARKSPEC_RES_SIZE);
            for (int i = 0; i < _BARKSPEC_RES_SIZE; i++) { bs_header[i] = "barkSpec_" + std::to_string(i+1); }
            header.insert(header.end(),bs_header.begin(),bs_header.end());
        }
        if (USE_BFCC) {
            std::vector<std::string> bfcc_header;
            bfcc_header.resize(_BFCC_RES_SIZE);
            for (int i = 0; i < _BFCC_RES_SIZE; i++) { bfcc_header[i] = "bfcc_" + std::to_string(i+1); }
            header.insert(header.end(),bfcc_header.begin(),bfcc_header.end());
        }
        if (USE_CEPSTRUM) {
            std::vector<std::string> cepstrum_header;
            cepstrum_header.resize(_CEPSTRUM_RES_SIZE);
            for (int i = 0; i < _CEPSTRUM_RES_SIZE; i++) { cepstrum_header[i] = "cepstrum_" + std::to_string(i+1); }
            header.insert(header.end(),cepstrum_header.begin(),cepstrum_header.end());
        }
        if (USE_MFCC) {
            std::vector<std::string> mfcc_header;
            mfcc_header.resize(_MFCC_RES_SIZE);
            for (int i = 0; i < _MFCC_RES_SIZE; i++) { mfcc_header[i] = "mfcc_" + std::to_string(i+1); }
            header.insert(header.end(),mfcc_header.begin(),mfcc_header.end());
        }
        if (USE_PEAKSAMPLE) {
            const std::vector<std::string> ps_header = {"peakSample_value","peakSample_index"};
            header.insert(header.end(),ps_header.begin(),ps_header.end());
        }
        if (USE_ZEROCROSSING) {
            header.push_back("zeroCrossing");
        }
    }
public:

    // Constructor
    FeatureExtractors() {
        barkSpecRes.resize(_BARKSPEC_RES_SIZE);
        bfccRes.resize(_BFCC_RES_SIZE);
        cepstrumRes.resize(_CEPSTRUM_RES_SIZE);
        mfccRes.resize(_MFCC_RES_SIZE);
        
        attackTime.setMaxSearchRange(20);
    }

    constexpr static size_t getFeVectorSize() {
        return VECTOR_SIZE;
    }

    std::vector<std::string> getHeader() {
        if (header.size() == 0)
            createHeader();
        return header;
    }

    void prepare(double sampleRate, unsigned int samplesPerBlock) {
        /** Prepare feature extractors **/
        bfcc.prepare(sampleRate, (uint32_t)samplesPerBlock);
        cepstrum.prepare(sampleRate, (uint32_t)samplesPerBlock);
        attackTime.prepare(sampleRate, (uint32_t)samplesPerBlock);
        barkSpecBrightness.prepare(sampleRate, (uint32_t)samplesPerBlock);
        barkSpec.prepare(sampleRate, (uint32_t)samplesPerBlock);
        mfcc.prepare(sampleRate, (uint32_t)samplesPerBlock);
        peakSample.prepare(sampleRate, (uint32_t)samplesPerBlock);
        zeroCrossing.prepare(sampleRate, (uint32_t)samplesPerBlock);
    }

    void reset() {
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

    template <typename SampleType>
    void store(AudioBuffer<SampleType>& buffer, short int channel) {
        if(USE_BFCC)
            bfcc.store(buffer,channel);
        if(USE_CEPSTRUM)
            cepstrum.store(buffer,channel);
        if(USE_ATTACKTIME)
            attackTime.store(buffer,channel);
        if(USE_BARKSPECBRIGHTNESS)
            barkSpecBrightness.store(buffer,channel);
        if(USE_BARKSPEC)
            barkSpec.store(buffer,channel);
        if(USE_MFCC)
            mfcc.store(buffer,channel);
        if(USE_PEAKSAMPLE)
            peakSample.store(buffer,channel);
        if(USE_ZEROCROSSING)
            zeroCrossing.store(buffer,channel);
    }

    std::string getInfoString(FE::Extractor extractor) {
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

    void computeFeatureVector(float featureVector[])
    {
        int last = -1;
        int newLast = 0;

        if (USE_ATTACKTIME) {
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
            info += ("attackTime [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
           #endif
            last = newLast;
        }

        if (USE_BARKSPECBRIGHTNESS) {
            /*-----------------------------------------/
            | 02 - Bark Spectral Brightness            |
            /-----------------------------------------*/
            float bsb = this->barkSpecBrightness.compute();

            featureVector[3] = bsb;
            newLast = 3;
           #ifdef LOG_SIZES
            info += ("barkSpecBrightness [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
           #endif
            last = newLast;
        }

        if (USE_BARKSPEC) {
            /*-----------------------------------------/
            | 03 - Bark Spectrum                       |
            /-----------------------------------------*/
            barkSpecRes = this->barkSpec.compute();

            assert(barkSpecRes.size() == _BARKSPEC_RES_SIZE);
            for(int i=0; i<_BARKSPEC_RES_SIZE; ++i)
            {
                featureVector[(last+1) + i] = barkSpecRes[i];
            }
            newLast = last + _BARKSPEC_RES_SIZE;
        #ifdef LOG_SIZES
            info += ("barkSpec [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
        #endif
            last = newLast;
        }

        if (USE_BFCC) {
            /*------------------------------------------/
            | 04 - Bark Frequency Cepstral Coefficients |
            /------------------------------------------*/
            bfccRes = this->bfcc.compute();
            assert(bfccRes.size() == _BFCC_RES_SIZE);
            for(int i=0; i<_BFCC_RES_SIZE; ++i)
            {
                featureVector[(last+1) + i] = bfccRes[i];
            }
            newLast = last + _BFCC_RES_SIZE;
           #ifdef LOG_SIZES
            info += ("bfcc [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
           #endif
            last = newLast;
        }

        if (USE_CEPSTRUM) {
            /*------------------------------------------/
            | 05 - Cepstrum Coefficients                |
            /------------------------------------------*/
            cepstrumRes = this->cepstrum.compute();
            assert(cepstrumRes.size() == _CEPSTRUM_RES_SIZE);
            for(int i=0; i<_CEPSTRUM_RES_SIZE; ++i)
            {
                featureVector[(last+1) + i] = cepstrumRes[i];
            }
            newLast = last + _CEPSTRUM_RES_SIZE;
           #ifdef LOG_SIZES
            info += ("cepstrum [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
           #endif
            last = newLast;
        }

        if (USE_MFCC) {
            /*-----------------------------------------/
            | 06 - Mel Frequency Cepstral Coefficients |
            /-----------------------------------------*/
            mfccRes = this->mfcc.compute();
            assert(mfccRes.size() == _MFCC_RES_SIZE);
            for(int i=0; i<_MFCC_RES_SIZE; ++i)
            {
                featureVector[(last+1) + i] = mfccRes[i];
            }
            newLast = last + _MFCC_RES_SIZE;
           #ifdef LOG_SIZES
            info += ("mfcc [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
           #endif
            last = newLast;
        }

        if (USE_PEAKSAMPLE) {
            /*-----------------------------------------/
            | 07 - Peak sample                         |
            /-----------------------------------------*/
            std::pair<float, unsigned long int> peakSample = this->peakSample.compute();
            float peakSampleRes = peakSample.first;
            unsigned long int peakSampleIndex = peakSample.second;
            featureVector[last+1] = peakSampleRes;
            featureVector[last+2] = peakSampleIndex;
            newLast = last + 2;
           #ifdef LOG_SIZES
            info += ("peakSample [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
           #endif
            last = newLast;
        }

        if (USE_ZEROCROSSING) {
            /*-----------------------------------------/
            | 08 - Zero Crossings                      |
            /-----------------------------------------*/
            uint32_t crossings = this->zeroCrossing.compute();
            featureVector[last+1] = crossings;
            newLast = last +1;
           #ifdef LOG_SIZES
            info += ("zeroCrossing [" + std::to_string(last+1) + ", " + std::to_string(newLast) + "]\n");
           #endif
            last = newLast;
        }

    }

};

}