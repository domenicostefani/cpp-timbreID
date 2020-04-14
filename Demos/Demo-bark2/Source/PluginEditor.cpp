/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - bark Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 26th March 2020

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <iostream>
#include <string>

//==============================================================================
DemoEditor::DemoEditor (DemoProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (500, 500);
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Bark Onset Detector", NotificationType::dontSendNotification);
    titleLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabel();

    addAndMakeVisible(barkLed);

    addAndMakeVisible(testButton);
    testButton.setButtonText("LED test button");
    testButton.addListener(this);

    addAndMakeVisible(boxDebounce);
    boxDebounce.setLabelText("Debounce time (millis) [0-inf]: ");
    boxDebounce.setListener(this);

    addAndMakeVisible(boxThresh);
    boxThresh.setLabelText("Thresholds (low,high): ");
    boxThresh.setListener(this);


    pollingTimer.startPolling();
}

DemoEditor::~DemoEditor()
{
}

//==============================================================================
void DemoEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    this->mSampleRate = processor.getSampleRate();
    this->mBlockSize = processor.getBlockSize();
    updateDataLabel();
}

void DemoEditor::resized()
{
    Rectangle<int> area = getLocalBounds();
    Rectangle<int> usable = area.reduced(10);

    Rectangle<int> bottom = usable.removeFromBottom(usable.getHeight()*0.3);
    Rectangle<int> data = usable.removeFromBottom(usable.getHeight()*0.5);
    Rectangle<int> top = usable;

    dataLabel.setBounds(data.removeFromLeft(data.getWidth()*0.5));
//    resLabel.setBounds(data);

    titleLabel.setBounds(top.removeFromTop(top.getHeight()/3));
    testButton.setBounds((top.removeFromRight(top.getHeight()*2)).reduced(5));
    barkLed.setBounds(top.reduced(7));

    int boxHeight = 25;
    boxDebounce.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxDebounce.getHeight()));
    boxThresh.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxDebounce.getHeight()));
}


void DemoEditor::buttonClicked (Button * button)
{
    if(button == &this->testButton)
    {
     this->barkLed.switchOn();
    }
    else if(boxDebounce.hasButton(button))
    {
         unsigned int millis = boxDebounce.getText().getIntValue();
         std::cout << "Setting debounce time to " << millis << " milliseconds" << std::endl;

         processor.bark.setDebounce(millis);
    }
    else if(boxThresh.hasButton(button))
    {
         float lo = boxThresh.getText1().getFloatValue();
         float hi = boxThresh.getText2().getFloatValue();
         std::cout << "Setting thresholds to " << lo << ", " << hi << std::endl;

         processor.bark.setThresh(lo,hi);
    }
}

void DemoEditor::updateDataLabel(){
    std::string sampleRate_s = std::to_string((int)this->mSampleRate);
    std::string blockSize_s = std::to_string((int)this->mBlockSize);
    std::string windowSize_s = std::to_string((int)this->processor.bark.getWindowSize());

    std::string text = "Sample rate:  "+sampleRate_s+"\nBlock size:  "+blockSize_s+"\nWindow size:  " +windowSize_s;
    text += "\nDebug mode: ";
    text += processor.bark.getDebugMode()?"True":"False";
    text += "\nSpew mode: ";
    text += processor.bark.getSpewMode()?"True":"False";
    text += "\nUse Weights: ";
    text += processor.bark.getUseWeights()?"True":"False";

    text += "\nDebounce time: ";
    text += std::to_string(processor.bark.getDebounceTime());
    text += " millis";

    text += "\nThresholds: ";
    auto threshs = processor.bark.getThresh();
    text += std::to_string(threshs.first);
    text += ", ";
    text += std::to_string(threshs.second);


    this->dataLabel.setText(text,NotificationType::dontSendNotification);
}

//void DemoEditor::updateResLabel(void<float>* growth, float* totalGrowth){
//    std::string totalGrowth_s = "NULL";
//    std::string growth_s = "NULL";
//    if(growth != nullptr && totalGrowth != nullptr)
//    {
//        totalGrowth_s = std::to_string(*totalGrowth);
//        growth_s = "";
//        for(int i=0; i<growth->size(), ++i)
//        {
//            growth_s += std::to_string(growth->at(i)]);
//            if(i != growth->size()-1)
//                growth_s += ", ";
//        }
//    }
//
//    std::string text = "TotalGrowth:  "+totalGrowth_s+"\nGrowthList: "+growth_s;
//
//    this->resLabel.setText(text,NotificationType::dontSendNotification);
//}
