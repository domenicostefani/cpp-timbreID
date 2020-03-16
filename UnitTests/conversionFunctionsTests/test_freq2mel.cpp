#include "UnitTest++.h"
#include "freq2mel.hpp"
#include "tIDLib.hpp"

namespace
{
SUITE(TestFreq2Mel){
    TEST(Freq0)
    {
        const float res = Freq2mel::calculate(0.0f);
        CHECK_CLOSE(0.0f,res,0.1f);
    }

    TEST(Freq1000)
    {
        const float res = Freq2mel::calculate(1000.0f);
        CHECK_CLOSE(999.99f,res,0.01f);
    }

    TEST(FreqMax)
    {
        const float res = Freq2mel::calculate(MAXMELFREQ);
        CHECK_CLOSE(MAXMELS,res,0.001f);
    }

    TEST(FreqNeg)
    {
        CHECK_THROW(Freq2mel::calculate(-1.0f),std::domain_error);
    }

    TEST(FreqOver)
    {
        CHECK_THROW(Freq2mel::calculate(MAXMELFREQ + 1.0f),std::domain_error);
    }
}
}
