/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - bark Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LED.h"
#include "setParamBox.h"

//==============================================================================
/**
*/
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

    LED barkLed;
    setParamBox boxDebounce;
    setDualParamBox boxThresh;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DemoProcessor& processor;

    Label titleLabel;
    Label dataLabel;
    Label resLabel;

    double mSampleRate = 0.0;
    int mBlockSize = 0;

    TextButton testButton;

    void updateDataLabel();
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
        if(processor.onsetMonitorState.exchange(false))
            barkLed.switchOn();
        updateDataLabel();
    }

    PollingTimer pollingTimer{[this]{pollingRoutine();}};
};
