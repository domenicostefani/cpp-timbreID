/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - peakSample Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 26th March 2020

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
    computeButton.setButtonText("Compute Peak sample");
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
        std::pair<float, unsigned long int> peakSample = processor.computePeakSample();
        this->mSample = peakSample.first;
        this->mIndex = peakSample.second;
        std::cout << "Sample value: " << mSample << " Index: " << mIndex << std::endl;
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
    std::string sample_s = std::to_string(this->mSample);
    std::string index_s = std::to_string((int)this->mIndex);
    this->resLabel.setText("Peak Sample\nValue: " +sample_s+"\nIndex: " + index_s,NotificationType::dontSendNotification);
}
