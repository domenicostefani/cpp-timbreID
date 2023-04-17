#include <gtest/gtest.h>
#include "../Source/postOnsetTimer.h"

#define DEF_SAMPLE_RATE 48000
#define DEF_BLOCK_SIZE 64

TEST(PostOnsetTimerTest, noprepare)
{
    PostOnsetTimer pot;
    ASSERT_THROW(pot.start(10),std::logic_error);
}

TEST(PostOnsetTimerTest, initialization)
{
    PostOnsetTimer pot;
    pot.prepare(DEF_SAMPLE_RATE,DEF_BLOCK_SIZE);
    ASSERT_TRUE(pot.isIdle());
    ASSERT_FALSE(pot.isStarted());
    ASSERT_FALSE(pot.isExpired());
}

TEST(PostOnsetTimerTest, start)
{
    PostOnsetTimer pot;
    pot.prepare(DEF_SAMPLE_RATE,DEF_BLOCK_SIZE);
    if (pot.isIdle())
        pot.start(64);

    ASSERT_FALSE(pot.isIdle());
    ASSERT_TRUE(pot.isStarted());
    ASSERT_FALSE(pot.isExpired());
}

TEST(PostOnsetTimerTest, expire)
{
    PostOnsetTimer pot;
    pot.prepare(DEF_SAMPLE_RATE,DEF_BLOCK_SIZE);
    const int DURATION_MS = 1000;

    if (pot.isIdle())
        pot.start(DURATION_MS);

    ASSERT_TRUE(pot.isStarted());

    for(int i=0; i < 1000; ++i)
    {
        if (1.0 * DEF_BLOCK_SIZE*i/DEF_SAMPLE_RATE*1000.0 < DURATION_MS)
        {
            ASSERT_FALSE(pot.isExpired());
        }
        else
        {
            ASSERT_TRUE(pot.isExpired());
            break;
        }
        pot.updateTimer();
    }

    ASSERT_TRUE(pot.isIdle());
    ASSERT_FALSE(pot.isStarted());
    ASSERT_FALSE(pot.isExpired());
}

TEST(PostOnsetTimerTest, reset)
{
    const int DURATION_MS = 1000;

    PostOnsetTimer pot;
    pot.prepare(DEF_SAMPLE_RATE,DEF_BLOCK_SIZE);
    pot.reset();

    ASSERT_TRUE(pot.isIdle());
    ASSERT_FALSE(pot.isStarted());
    ASSERT_FALSE(pot.isExpired());

    pot.prepare(DEF_SAMPLE_RATE,DEF_BLOCK_SIZE);
    if (pot.isIdle())
        pot.start(1000);
    pot.reset();

    ASSERT_TRUE(pot.isIdle());

    ASSERT_FALSE(pot.isStarted());
    ASSERT_FALSE(pot.isExpired());
}

TEST(PostOnsetTimerTest, negativeDuration)
{
    const int DURATION_MS = -1000;

    PostOnsetTimer pot;
    pot.prepare(DEF_SAMPLE_RATE,DEF_BLOCK_SIZE);
    if (pot.isIdle())
        ASSERT_THROW(pot.start(DURATION_MS),\
                     std::logic_error);
}

TEST(PostOnsetTimerTest, negativeBlockSize)
{
    const int DURATION_MS = 1000,
              BLOCK_SIZE = -64;

    PostOnsetTimer pot;
    ASSERT_THROW(pot.prepare(DEF_SAMPLE_RATE,BLOCK_SIZE),\
                 std::logic_error);
}

TEST(PostOnsetTimerTest, negativeSampleRate)
{
    const int DURATION_MS = 1000,
              SAMPLE_RATE = -48000;

    PostOnsetTimer pot;

    ASSERT_THROW(pot.prepare(SAMPLE_RATE,DEF_BLOCK_SIZE),\
                 std::logic_error);
}

TEST(PostOnsetTimerTest, startFromStarted)
{
    const int DURATION_MS = 1000,
              BLOCK_SIZE = 64,
              SAMPLE_RATE = 48000;

    PostOnsetTimer pot;
    pot.prepare(DEF_SAMPLE_RATE,DEF_BLOCK_SIZE);
    if (pot.isIdle())
        pot.start(DURATION_MS);
    ASSERT_THROW(pot.start(DURATION_MS),\
                    std::logic_error);
}

TEST(PostOnsetTimerTest, doubleExpired)
{
    PostOnsetTimer pot;
    pot.prepare(DEF_SAMPLE_RATE,DEF_BLOCK_SIZE);
    const int DURATION_MS = 1000;

    if (pot.isIdle())
        pot.start(DURATION_MS);

    ASSERT_TRUE(pot.isStarted());

    for(int i=0; i < 1000; ++i)
    {
        if (1.0 * DEF_BLOCK_SIZE*i/DEF_SAMPLE_RATE*1000.0 < DURATION_MS)
        {
            ASSERT_FALSE(pot.isExpired());
        }
        else
        {
            ASSERT_TRUE(pot.isExpired());
            ASSERT_FALSE(pot.isExpired());
            break;
        }
        pot.updateTimer();
    }

    ASSERT_TRUE(pot.isIdle());
    ASSERT_FALSE(pot.isStarted());
    ASSERT_FALSE(pot.isExpired());
}

int main(int argc, char **argv){
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
