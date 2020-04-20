/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - bark Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 15th April 2020

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
    void updateResLabel(float brightness);

    SetParamBox windowSizeBox, boundaryBox;

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
