#include "UnitTest++.h"
#include "bark2freq.hpp"
#include "tIDLib.hpp"

namespace
{
SUITE(TestBark2Freq){
    TEST(Bark1)
    {
        const float res = Bark2freq::calculate(1.0f);
        CHECK_CLOSE(100.5,res,0.1);
    }

    TEST(Bark2)
    {
        const float res = Bark2freq::calculate(2.0f);
        CHECK_CLOSE(203.7,res,0.1);
    }

    TEST(Bark3)
    {
        const float res = Bark2freq::calculate(3.0f);
        CHECK_CLOSE(312.7,res,0.1);
    }

    TEST(BarkMax)
    {
        const float res = Bark2freq::calculate(MAXBARKS);
        CHECK_CLOSE(MAXBARKFREQ,res,0.1);
    }

    TEST(BarkNeg)
    {
        CHECK_THROW(Bark2freq::calculate(-1.0f),std::domain_error);
    }

    TEST(BarkOver)
    {
        CHECK_THROW(Bark2freq::calculate(MAXBARKS + 10.0f),std::domain_error);
    }
}
}
