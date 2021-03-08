/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - Feature Extractor Plugin

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 10th September 2020

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "LED.h"
#include "setParamBox.h"


#define EXTRACTOR_DATA_SIZE  8

//==============================================================================
/**
*/
class DemoEditor  : public AudioProcessorEditor,
                    public Button::Listener,
                    private ComboBox::Listener
{
public:
    DemoEditor (DemoProcessor&);
    ~DemoEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void buttonClicked (Button *) override;
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

    LED onsetLed;

    setParamBox boxIoI;
    setParamBox boxThresh;
    setParamBox boxSilenceThresh;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    DemoProcessor& processor;

    //
    // Onset detector section
    //

    Label titleLabel;
    Label dataLabel;
    Label extractorData[EXTRACTOR_DATA_SIZE];

    double mSampleRate = 0.0;
    int mBlockSize = 0;

    void updateDataLabels();
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
        int interval = 3;
    };

    void pollingRoutine()
    {
        if(processor.onsetMonitorState.exchange(false))
            onsetLed.switchOn();
        updateDataLabels();

        int onsetCounter = -1;
        // do { //TODO: figure out why I did this
            onsetCounter = processor.onsetCounterAtomic.load(std::memory_order_relaxed);  // get value atomically
        // } while (onsetCounter==-1);

        updateOnsetCounter(onsetCounter);
    }

    PollingTimer pollingTimer{[this]{pollingRoutine();}};


    //
    // Storage section
    //
    Label storageTitle;
    ComboBox storageStateBox;
    TextButton clearAll;
    setParamBox write;
    Label dispInfo;
    void updateOnsetCounter(unsigned int onsetCounter);
};
