#include "UnitTest++.h"
#include "mel2freq.hpp"
#include "tIDLib.hpp"

namespace
{
SUITE(TestMel2Freq){
    TEST(Mel0)
    {
        const float res = Mel2freq::calculate(0.0f);
        CHECK_CLOSE(0.0f,res,0.1f);
    }

    TEST(Mel1000)
    {
        const float res = Mel2freq::calculate(999.99f);
        CHECK_CLOSE(1000.0f,res,0.1f);
    }

    TEST(MelMax)
    {
        const float res = Mel2freq::calculate(MAXMELS);
        CHECK_CLOSE(MAXMELFREQ,res,0.1f);
    }

    TEST(MelNeg)
    {
        CHECK_THROW(Mel2freq::calculate(-1.0f),std::domain_error);
    }

    TEST(MelOver)
    {
        CHECK_THROW(Mel2freq::calculate(MAXMELS + 1.0f),std::domain_error);
    }
}
}
