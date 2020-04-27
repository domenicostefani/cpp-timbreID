/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - mfcc Module (Mel Frequency Cepstral Coefficients)

  Mel-frequency cepstrum is much different than raw cepstrum.
  The most significant differences are an emphasis on lower spectral content and
  the use of a DCT rather than a FT in the final step of the process. When mfcc~
  receives a bang, it spits out the MFCCs for the most recent analysis window as
  a list. The default 100mel filterbank spacing produces a 38-component MFCC
  vector regardless of window size. MFCC components are normalized to be between
  1 and -1 by default.
  If normalized, the first MFCC will always have a value of 1

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 27th April 2020

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "setParamBox.h"

//==============================================================================
class DemoEditor  : public AudioProcessorEditor,
                    public Button::Listener
{
public:
    DemoEditor (DemoProcessor&);
    ~DemoEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void buttonClicked (Button *);

private:
    DemoProcessor& processor;

    Label titleLabel;
    Label resLabel;
    Label dataLabel;

    TextButton computeBtn;

    void updateDataLabel();
    void updateResLabel(std::vector<float> &coeff);

    SetParamBox windowFuncBox,
                filterStateBox,
                filterOpBox,
                spectrumTypeBox,
                windowSizeBox,
                normalizeBox,
                createFilterbankBox;

    typedef std::function<void (void)> TimerEventCallback;
    class PollingTimer : public Timer
    {
    public:

        PollingTimer(TimerEventCallback cb)
        {
            this->cb = cb;
        }

        void startPolling()
        {
            Timer::startTimer(this->interval);
        }

        void timerCallback() override
        {
            cb();
        }
    private:
        TimerEventCallback cb;
        int interval = 33;
    };

    void pollingRoutine()
    {
        updateDataLabel();
    }

    PollingTimer pollingTimer{[this]{pollingRoutine();}};
};
