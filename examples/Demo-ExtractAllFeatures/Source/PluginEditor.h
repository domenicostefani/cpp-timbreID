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


typedef juce::AudioProcessorValueTreeState::ComboBoxAttachment ComboBoxAttachment;
typedef juce::AudioProcessorValueTreeState::ButtonAttachment ButtonAttachment;

//==============================================================================
/**
*/
class DemoEditor  : public AudioProcessorEditor,
                    private ComboBox::Listener
{
public:
    DemoEditor (DemoProcessor&);
    ~DemoEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;
    void comboBoxChanged(ComboBox* comboBoxThatHasChanged) override;

    LED onsetLed;

    setParamBox boxIoI;
    setParamBox boxThresh;
    setParamBox boxSilenceThresh;

    
    std::unique_ptr<ComboBoxAttachment> storestateAttachment;
    std::unique_ptr<ButtonAttachment>   clearAttachment,
                                        savefileAttachment;

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

        int onsetCounter = processor.onsetCounterAtomic.load(std::memory_order_relaxed);  // get value atomically

        updateOnsetCounter(onsetCounter);
        performStoreClearActions();
    }

    PollingTimer pollingTimer{[this]{pollingRoutine();}};


    //
    // Storage section
    //
    Label storageTitle;
    ComboBox storageStateBox;
    TextButton clearAll;
    setDualParamBox write {"SaveFile"};
    Label dispInfo;
    void updateOnsetCounter(unsigned int onsetCounter);
    std::string dirpath;


    void performStoreClearActions();
};
