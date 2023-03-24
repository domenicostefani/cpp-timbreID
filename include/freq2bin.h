#pragma once

namespace tid   /* TimbreID namespace*/
{
class Freq2bin
{
public:
    static float calculate(float freq);
    static void setWinSampRate(long int windowSize, long int sampleRate);
    static unsigned long int getWindowSize();
    static unsigned long int getSampleRate();
private:
    static unsigned long int windowSize;
    static unsigned long int sampleRate;
};
} // namespace tid
