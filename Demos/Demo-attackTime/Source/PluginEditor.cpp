/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - attackTime Module

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
    setSize (500, 300);
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


    addAndMakeVisible(setSearchRangeBox);
    setSearchRangeBox.addListener(this);
    addAndMakeVisible(setSampMagThreshBox);
    setSampMagThreshBox.addListener(this);
    setSampMagThreshBox.setFloat(true);
    addAndMakeVisible(setNumSampsThreshBox);
    setNumSampsThreshBox.addListener(this);
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

    Rectangle<int> bottom = usable.removeFromBottom(usable.getHeight()*0.1);
    Rectangle<int> data = usable.removeFromBottom(usable.getHeight()*0.5);
    Rectangle<int> top = usable;

    computeButton.setBounds(bottom);
    dataLabel.setBounds(data.removeFromLeft(data.getWidth()*0.5));
    resLabel.setBounds(data);

    setSearchRangeBox.setBounds(top.removeFromTop(top.getHeight()/3));
    setSampMagThreshBox.setBounds(top.removeFromTop(top.getHeight()/2));
    setNumSampsThreshBox.setBounds(top.removeFromTop(top.getHeight()));
}

void DemoEditor::buttonClicked (Button * button){
    if(button == &this->computeButton)
    {
        processor.computeAttackTime(&this->mPeakSampIdx,&this->mAttackStartIdx,&this->mAttackTime);

        std::cout << "IDX peak sample: " << mPeakSampIdx << " IDX attack start: " << mAttackStartIdx << " attack TIME: " << mAttackTime << std::endl;
        updateDataLabel();
        updateResLabel();
    }else if(button == &this->setSearchRangeBox.setButton){
        unsigned long int millis = setSearchRangeBox.inputText.getText().getIntValue();
        std::cout << "Setting search range to " << millis << " milliseconds" << std::endl;

        processor.attackTime.setMaxSearchRange(millis);
    }else if(button == &this->setSampMagThreshBox.setButton){
        float value = setSampMagThreshBox.inputText.getText().getFloatValue();
        std::cout << "Setting SampMagThreshBox to " << value << " " << std::endl;

        processor.attackTime.setSampMagThresh(value);
    }else if(button == &this->setNumSampsThreshBox.setButton){
        unsigned long int value = setNumSampsThreshBox.inputText.getText().getIntValue();
        std::cout << "Setting NumSampsThreshBox to " << value << " " << std::endl;

        processor.attackTime.setNumSampsThresh(value);
    }
}

void DemoEditor::updateDataLabel(){
    std::string sampleRate_s = std::to_string((int)this->mSampleRate);
    std::string blockSize_s = std::to_string((int)this->mBlockSize);
    std::string windowSize_s = std::to_string((int)this->processor.attackTime.getWindowSize());
    std::string maxSearchRange_s = std::to_string((int)this->processor.attackTime.getMaxSearchRange());
    std::string msrMillis_s = std::to_string((int)(processor.attackTime.getMaxSearchRange()/this->mSampleRate*1000));
    this->dataLabel.setText("Sample rate:  "+sampleRate_s+"\nBlock size:  "+blockSize_s+"\nWindow size:  " +windowSize_s+"\nMax search range:  \n\t" +maxSearchRange_s+" samples\n\t"+msrMillis_s+" millis",NotificationType::dontSendNotification);

    setSearchRangeBox.update("Search Range",processor.attackTime.getMaxSearchRange()/processor.getSampleRate()*1000);
    setSampMagThreshBox.update("SampMagThresh", processor.attackTime.getSampMagThresh());
    setNumSampsThreshBox.update("SampMagThresh", processor.attackTime.getNumSampsThresh());
}

void DemoEditor::updateResLabel(){
    std::string peakSampIdx_s = std::to_string(this->mPeakSampIdx);
    std::string attackStartIdx_s = std::to_string(this->mAttackStartIdx);
    std::string attackTime_s = std::to_string(this->mAttackTime);
    this->resLabel.setText("Peak Sample index: " +peakSampIdx_s+"\nAttack Start index: " + attackStartIdx_s+"\nAttack time: "+attackTime_s,NotificationType::dontSendNotification);
}
