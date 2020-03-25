/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - zeroCrossing Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <iostream>


//==============================================================================
DemoEditor::DemoEditor (DemoProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    setSize (300, 200);
    // Show Button
    addAndMakeVisible(computeButton);
    computeButton.setButtonText("Compute zero crossings");
    computeButton.addListener(this);
    // Show Data Label
    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabel();
    // Show Results Label
    addAndMakeVisible(resLabel);
    resLabel.setJustificationType(Justification::topLeft);
    updateResLabel();
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

    Rectangle<int> bottom = usable.removeFromBottom(usable.getHeight()*0.5);
    Rectangle<int> top = usable.removeFromBottom(usable.getHeight()*0.8);

    computeButton.setBounds(bottom.reduced(10));
    dataLabel.setBounds(top.removeFromLeft(top.getWidth()*0.5));
    resLabel.setBounds(top);
}

void DemoEditor::buttonClicked (Button * button){
    if(button == &this->computeButton)
    {
        bgColor = Colours::aliceblue;
        uint32 crossings = processor.computeZeroCrossing();
        std::cout << "crossings: " << crossings << std::endl;
        this->mCrossings = crossings;
        updateDataLabel();
        updateResLabel();
    }
}

void DemoEditor::updateDataLabel(){
    std::string sampleRate_s = std::to_string((int)this->mSampleRate);
    std::string blockSize_s = std::to_string((int)this->mBlockSize);
    std::string windowSize_s = std::to_string((int)this->processor.getWindowSize());
    this->dataLabel.setText("Sample rate:  "+sampleRate_s+"\nBlock size:  "+blockSize_s+"\nWindow size:  " +windowSize_s,NotificationType::dontSendNotification);
}

void DemoEditor::updateResLabel(){
    std::string crossings_s = std::to_string((int)this->mCrossings);
    double freq = mSampleRate*mCrossings/(this->processor.getWindowSize()*2);
    std::string freq_s = std::to_string(freq);
    this->resLabel.setText("# Crossings " +crossings_s+"\nEquiv. frequency: " + freq_s,NotificationType::dontSendNotification);
}
