/*
  ==============================================================================
     Squared Sinusoid-wave Tone generator

     Author: Domenico Stefani
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class SquaredSine {
public:
    SquaredSine() {}

    ~SquaredSine() {}

    void prepareToPlay(int sampleRate) {
        this->sampleRate = sampleRate;

        SQUARED_SINE_MAX_AMPLITUDE = 1;
        amplitude = 0;
        wavetableSize = 64;

        for (size_t i = 0; i < this->wavetableSize; ++i) {
            // wavetable.insert(i, sinf(double_Pi * i / wavetableSize)*sinf(double_Pi * i / wavetableSize));
            wavetable.insert(i, sinf(2.0 * double_Pi * i / wavetableSize) * sinf(2.0 * double_Pi * i / wavetableSize));
        }
    }

    void processBlock(AudioBuffer<float> &buffer, int channel = 0) {
        uint32 startSample = 0;
        for (int sample = 0; sample < buffer.getNumSamples(); ++sample) {
            float *const bufferWritePtr = buffer.getWritePointer(channel, startSample);
            const float *bufferReadPtr = buffer.getReadPointer(channel, startSample);

            bufferWritePtr[sample] = bufferReadPtr[sample] + (wavetable[sample] * amplitude);
        }
        amplitude = 0;
    }

    void playHalfwave() {
        this->amplitude = SQUARED_SINE_MAX_AMPLITUDE;
    }

private:
    Array<double> wavetable;
    double wavetableSize;
    double amplitude;
    double SQUARED_SINE_MAX_AMPLITUDE;
    int sampleRate;
};
