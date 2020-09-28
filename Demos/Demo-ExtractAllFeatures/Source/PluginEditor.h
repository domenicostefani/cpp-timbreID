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

   #ifdef USE_AUBIO_ONSET
    setParamBox boxIoI;
    setParamBox boxThresh;
    setParamBox boxSilenceThresh;
   #else
    setParamBox boxDebounce;
    setDualParamBox boxThresh;
   #endif

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
        int interval = 33;
    };

    void pollingRoutine()
    {
        if(processor.onsetMonitorState.exchange(false))
            onsetLed.switchOn();
        updateDataLabels();

        int match = -1;
        do {
            match = processor.matchAtomic.load(std::memory_order_relaxed);  // get value atomically
        } while (match==-1);

        float distance = -2;
        do {
            distance = processor.matchAtomic.load(std::memory_order_relaxed);  // get value atomically
        } while (distance==-2);

        updateKnnLabel(match,distance);
    }

    PollingTimer pollingTimer{[this]{pollingRoutine();}};


    //
    // KNN (TimbreID) section
    //
    Label classifierTitle;
    ComboBox classifierState;
    TextButton uncluster,clearAll;
    setParamBox cluster,write,read, writeText, readText,manualCluster;
    Label dispTimbre;
    Label dispDist;
    void updateKnnLabel(unsigned int match, float distance);
};
