#include "UnitTest++.h"
#include "bin2freq.hpp"

namespace
{
SUITE(TestBin2Freq){
    TEST(Bin0)
    {
        Bin2freq::setWinSampRate(512,44100);
        const float res = Bin2freq::calculate(0);
        CHECK_CLOSE(0.0,res,0.1);
    }

    TEST(Bin1)
    {
        Bin2freq::setWinSampRate(512,44100);
        const float res = Bin2freq::calculate(1);
        CHECK_CLOSE(86.13,res,0.1);
    }

    TEST(DifferentWinSR)
    {
        Bin2freq::setWinSampRate(1024,100);
        const float res = Bin2freq::calculate(1);
        CHECK_CLOSE(0.097,res,0.001);
    }

    TEST(BinNeg)
    {
        Bin2freq::setWinSampRate(512,44100);
        CHECK_THROW(Bin2freq::calculate(-1),std::domain_error);   // Op fails because bin < 0 (Good)
    }

    TEST(BinOver)
    {
        Bin2freq::setWinSampRate(512,44100);
        CHECK_THROW(Bin2freq::calculate(1024),std::domain_error);   // Op fails because bin > winSize (Good)
    }

    TEST(WinSizeNeg)
    {
        CHECK_THROW(Bin2freq::setWinSampRate(-1,44100), std::invalid_argument);   // Op fails because winSize <= 0 (Good)
    }

    TEST(WinSizeZero)
    {
        CHECK_THROW(Bin2freq::setWinSampRate(0,44100), std::invalid_argument);   // Op fails because winSize <= 0 (Good)
    }

    TEST(SampleRateNeg)
    {
        CHECK_THROW(Bin2freq::setWinSampRate(512,-1), std::invalid_argument);   // Op fails because SampleRate <= 0 (Good)
    }

    TEST(SampleRateZero)
    {
        CHECK_THROW(Bin2freq::setWinSampRate(512,0), std::invalid_argument);   // Op fails because SampleRate <= 0 (Good)
    }

    TEST(GetSetWinSize)
    {
        Bin2freq::setWinSampRate(256,48000);
        const unsigned long int res = Bin2freq::getWindowSize();
        CHECK_EQUAL((long unsigned int)256,res);
    }

    TEST(GetSetSampRate)
    {
        Bin2freq::setWinSampRate(256,192000);
        const unsigned long int res = Bin2freq::getSampleRate();
        CHECK_EQUAL((long unsigned int)192000,res);
    }


}
}
