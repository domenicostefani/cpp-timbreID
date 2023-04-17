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
    setSize (700, 650);
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Onset detector + bfcc + classifier", NotificationType::dontSendNotification);
    titleLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabel();

    addAndMakeVisible(barkLed);

    pollingTimer.startPolling();


    addAndMakeVisible(bfccTitle);
    bfccTitle.setText("BFCC extractor", NotificationType::dontSendNotification);

    addAndMakeVisible(bfccInfo);
    bfccInfo.setJustificationType(Justification::topLeft);

    addAndMakeVisible(classifierTitle);
    classifierTitle.setText("Classifier Section", NotificationType::dontSendNotification);
    classifierTitle.setJustificationType(Justification::centred);

    addAndMakeVisible(dispTimbre);  //Label
    dispTimbre.setText("dispTimbre", NotificationType::dontSendNotification);

    addAndMakeVisible(dispDist);  //Label
    dispDist.setText("dispDist", NotificationType::dontSendNotification);
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
    int boxHeight = 25;
    Rectangle<int> area = getLocalBounds();
    Rectangle<int> usable = area.reduced(10);

    titleLabel.setBounds(usable.removeFromTop(boxHeight*2));
    Rectangle<int> data = usable.removeFromTop(usable.getHeight()*0.3);
    Rectangle<int> bottom = usable;

    dataLabel.setBounds(data.removeFromLeft(data.getWidth()*0.5));
    barkLed.setBounds(data.reduced(15));

    int spacer = 25;
    int lateralWidth = (bottom.getWidth()-spacer)/2;
    Rectangle<int> bottomLeft = bottom.removeFromLeft(lateralWidth);
    Rectangle<int> bottomRight = bottom.removeFromRight(lateralWidth);

    // Bfcc module on the topLeft
    bfccTitle.setBounds(bottomLeft.removeFromTop(boxHeight*2));
    bfccInfo.setBounds(bottomLeft);

    // Knn classifier (timbreID on the right)
    classifierTitle.setBounds(bottomRight.removeFromTop(boxHeight*2));
    Rectangle<int> knnCommands = bottomRight.removeFromTop(bottomRight.getHeight()*0.8);
    Rectangle<int> knnDisplay = bottomRight;

    dispTimbre.setBounds(knnDisplay.removeFromTop(boxHeight));
    dispDist.setBounds(knnDisplay.removeFromTop(boxHeight));
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

void DemoEditor::updateBFCCDataLabel(){
    std::string text = processor.bfcc.getInfoString();
    text += "\n\nFeatures used: ";
    text += std::to_string(processor.featuresUsed);
    this->bfccInfo.setText(text,NotificationType::dontSendNotification);
}

void DemoEditor::updateKnnLabel(unsigned int match, float distance)
{
    this->dispTimbre.setText("Timbre: " + std::to_string(match), NotificationType::dontSendNotification);
    this->dispDist.setText("Distance: " + std::to_string(distance), NotificationType::dontSendNotification);
}
