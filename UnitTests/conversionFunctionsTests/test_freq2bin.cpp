#include "UnitTest++.h"
#include "freq2bin.hpp"
#include "tIDLib.hpp"

namespace
{
SUITE(TestFreq2Bin){
    TEST(Freq0)
    {
        Freq2bin::setWinSampRate(512,44100);
        const float res = Freq2bin::calculate(0.0f);
        CHECK_CLOSE(0.0f,res,0.1f);
    }

    TEST(Freq1000)
    {
        Freq2bin::setWinSampRate(512,44100);
        const float res = Freq2bin::calculate(1000.0f);
        CHECK_CLOSE(11.61f,res,0.01f);
    }

    TEST(DifferentWinSR)
    {
        Freq2bin::setWinSampRate(1024,100);
        const float res = Freq2bin::calculate(0.097);
        CHECK_CLOSE(1.000,res,0.01);
    }

    TEST(FreqNeg)
    {
        Freq2bin::setWinSampRate(512,44100);
        CHECK_THROW(Freq2bin::calculate(-1),std::domain_error);   // Op fails because freq < 0 (Good)
    }

    TEST(FreqOver)
    {
        Freq2bin::setWinSampRate(512,44100);
        CHECK_THROW(Freq2bin::calculate(48000),std::domain_error);   // Op fails because freq > sampling rate (Good)
    }

    TEST(WinSizeNeg)
    {
        CHECK_THROW(Freq2bin::setWinSampRate(-1,44100), std::invalid_argument);   // Op fails because winSize <= 0 (Good)
    }

    TEST(WinSizeZero)
    {
        CHECK_THROW(Freq2bin::setWinSampRate(0,44100), std::invalid_argument);   // Op fails because winSize <= 0 (Good)
    }

    TEST(SampleRateNeg)
    {
        CHECK_THROW(Freq2bin::setWinSampRate(512,-1), std::invalid_argument);   // Op fails because SampleRate <= 0 (Good)
    }

    TEST(SampleRateZero)
    {
        CHECK_THROW(Freq2bin::setWinSampRate(512,0), std::invalid_argument);   // Op fails because SampleRate <= 0 (Good)
    }

    TEST(GetSetWinSize)
    {
        Freq2bin::setWinSampRate(256,48000);
        const unsigned long int res = Freq2bin::getWindowSize();
        CHECK_EQUAL((long unsigned int)256,res);
    }

    TEST(GetSetSampRate)
    {
        Freq2bin::setWinSampRate(256,192000);
        const unsigned long int res = Freq2bin::getSampleRate();
        CHECK_EQUAL((long unsigned int)192000,res);
    }

}
}
