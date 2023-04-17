#pragma once

#include <JuceHeader.h>

#define MAX_AMPLITUDE 0.25;

class WavetableSine {
public:

    WavetableSine() {}

    ~WavetableSine() {}

    void prepareToPlay(int sampleRate)
    {
        this->sampleRate = sampleRate;

        phase = phaseAtBufferStart = 0;
        amplitude = 0;
        wavetableSize = 1024;
        increment = frequency * wavetableSize / sampleRate;


        for (size_t i = 0; i < this->wavetableSize; ++i) {
            wavetable.insert(i, sinf(2.0 * double_Pi * i / wavetableSize));
        }


        this->envelope.setSampleRate(sampleRate);

        envelopeParameters.attack = 0.1f;
        envelopeParameters.decay = 0.5f;
        envelopeParameters.sustain = 0.0f;
        envelopeParameters.release = 0.0f;
        this->envelope.setParameters(envelopeParameters);
    }

    void processBlock(AudioBuffer<float>& buffer) {
        uint32_t startSample = 0;
        phase = phaseAtBufferStart;
        for (int sample = 0; sample < buffer.getNumSamples();++sample) {
            updateFrequency();
            for (int channel = 0; channel < buffer.getNumChannels();++channel) {
                float* const bufferPtr = buffer.getWritePointer(channel, startSample);

                bufferPtr[sample] = wavetable[(int)phase] * amplitude * envelope.getNextSample();

            }
            phase = fmod(phase + increment, wavetableSize);
        }
        phaseAtBufferStart = phase;
    }

    void playNote(double frequency)
    {
        this->newFrequency = frequency;
        this->amplitude = MAX_AMPLITUDE;
        envelope.noteOn();
    }

    void stopNote()
    {
        envelope.noteOff();
    }

private:

    void updateFrequency() {
        frequency = newFrequency;
        increment = frequency * wavetableSize / sampleRate;
    }

    Array<double> wavetable;
    double wavetableSize,
        phase,
        phaseAtBufferStart,
        increment,
        frequency;
    uint32_t samplesPerBlock;
    double amplitude, newFrequency = 440;
    int sampleRate;

    ADSR envelope;
    ADSR::Parameters envelopeParameters;
};
