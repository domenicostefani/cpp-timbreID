#include "UnitTest++.h"
#include "freq2bark.hpp"
#include "tIDLib.hpp"

namespace
{
SUITE(TestFreq2Bark){

    TEST(Freq0)
    {
        const float res = Freq2bark::calculate(0.0f);
        CHECK_CLOSE(0.0f,res,0.1f);
    }

    TEST(Freq1000)
    {
        const float res = Freq2bark::calculate(1000.0f);
        CHECK_CLOSE(7.7f,res,0.1f);
    }

    TEST(FreqMax)
    {
        const float res = Freq2bark::calculate(MAXBARKFREQ);
        CHECK_CLOSE(MAXBARKS,res,0.001f);
    }

    TEST(FreqNeg)
    {
        CHECK_THROW(Freq2bark::calculate(-1.0f),std::domain_error);
    }

    TEST(FreqOver)
    {
        CHECK_THROW(Freq2bark::calculate(MAXBARKFREQ + 1.0f),std::domain_error);
    }

}
}
