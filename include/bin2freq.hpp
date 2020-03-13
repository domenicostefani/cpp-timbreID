#pragma once

class Bin2freq
{
public:
    static float calculate(float bin);
    static void setWinSampRate(long int windowSize, long int sampleRate);
    static unsigned long int getWindowSize();
    static unsigned long int getSampleRate();
private:
    static unsigned long int windowSize;
    static unsigned long int sampleRate;
};
