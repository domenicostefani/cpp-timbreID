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

    addAndMakeVisible(boxDebounce);
    boxDebounce.setLabelText("Debounce time (millis) [0-inf]: ");
    boxDebounce.addListener(this);

    addAndMakeVisible(boxThresh);
    boxThresh.setLabelText("Thresholds (low,high): ");
    boxThresh.addListener(this);


    pollingTimer.startPolling();


    addAndMakeVisible(bfccTitle);
    bfccTitle.setText("BFCC extractor", NotificationType::dontSendNotification);

    addAndMakeVisible(featureNumber);
    featureNumber.setLabelText("# Features Used:");
    featureNumber.setDefaultText("25");
    featureNumber.addListener(this);

    addAndMakeVisible(bfccInfo);
    bfccInfo.setJustificationType(Justification::topLeft);






    addAndMakeVisible(classifierTitle);
    classifierTitle.setText("Classifier Section", NotificationType::dontSendNotification);
    classifierTitle.setJustificationType(Justification::centred);

    addAndMakeVisible(classifierState);
    classifierState.addItem("Idle",3);
    classifierState.addItem("Train",1);
    classifierState.addItem("Classify",2);
    classifierState.setSelectedId(3);
    classifierState.setJustificationType(Justification::centred);
    classifierState.addListener(this);


    addAndMakeVisible(uncluster);  //TextButton
    uncluster.addListener(this);
    uncluster.setButtonText("uncluster");

    addAndMakeVisible(clearAll);  //TextButton
    clearAll.addListener(this);
    clearAll.setButtonText("clear");

    addAndMakeVisible(cluster);  //SetParamBox
    cluster.addListener(this);
    cluster.setLabelText("cluster");

    addAndMakeVisible(write);  //SetParamBox
    write.addListener(this);
    write.setLabelText("write");
    write.setDefaultText("./data/feature-db.timid");

    addAndMakeVisible(read);  //SetParamBox
    read.addListener(this);
    read.setLabelText("read");
    read.setDefaultText("./data/feature-db.timid");

    addAndMakeVisible(writeText);  //SetParamBox
    writeText.addListener(this);
    writeText.setLabelText("write Text");
    writeText.setDefaultText("./data/feature-db.csv");

    addAndMakeVisible(readText);  //SetParamBox
    readText.addListener(this);
    readText.setLabelText("read Text");
    readText.setDefaultText("./data/feature-db.csv");

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

    boxDebounce.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxDebounce.getHeight()));
    boxThresh.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxDebounce.getHeight()));
    bottom.removeFromTop(boxHeight*1.5);

    int spacer = 25;
    int lateralWidth = (bottom.getWidth()-spacer)/2;
    Rectangle<int> bottomLeft = bottom.removeFromLeft(lateralWidth);
    Rectangle<int> bottomRight = bottom.removeFromRight(lateralWidth);

    // Bfcc module on the topLeft
    bfccTitle.setBounds(bottomLeft.removeFromTop(boxHeight*2));
    featureNumber.setBounds(bottomLeft.removeFromTop(boxHeight));
    bfccInfo.setBounds(bottomLeft);

    // Knn classifier (timbreID on the right)
    classifierTitle.setBounds(bottomRight.removeFromTop(boxHeight*2));
    Rectangle<int> knnCommands = bottomRight.removeFromTop(bottomRight.getHeight()*0.8);
    Rectangle<int> knnDisplay = bottomRight;

    classifierState.setBounds(knnCommands.removeFromTop(boxHeight));
    cluster.setBounds(knnCommands.removeFromTop(boxHeight));
    uncluster.setBounds(knnCommands.removeFromTop(boxHeight));
    clearAll.setBounds(knnCommands.removeFromTop(boxHeight));
    knnCommands.removeFromTop(boxHeight*0.5);
    write.setBounds(knnCommands.removeFromTop(boxHeight));
    read.setBounds(knnCommands.removeFromTop(boxHeight));

    writeText.setBounds(knnCommands.removeFromTop(boxHeight));
    readText.setBounds(knnCommands.removeFromTop(boxHeight));

    dispTimbre.setBounds(knnDisplay.removeFromTop(boxHeight));
    dispDist.setBounds(knnDisplay.removeFromTop(boxHeight));
}


void DemoEditor::buttonClicked (Button * button)
{
    if(boxDebounce.hasButton(button))
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
    else if(cluster.hasButton(button))
    {
        std::cout << "cluster pressed" << std::endl;
        unsigned int numClusters = cluster.getText().getIntValue();
        processor.knn.autoCluster(numClusters);
    }
    else if(&uncluster == button)
    {
        std::cout << "uncluster pressed" << std::endl;
        processor.knn.uncluster();
    }
    else if(&clearAll == button)
    {
        std::cout << "clearAll pressed" << std::endl;
        processor.knn.clearAll();
    }
    else if(write.hasButton(button))
    {
        std::cout << "write pressed" << std::endl;
        std::string path = write.getText().toStdString();
        std::cout << "path " << path << std::endl;
        processor.knn.writeData(path);
    }
    else if(read.hasButton(button))
    {
        std::cout << "read pressed" << std::endl;
        std::string path = read.getText().toStdString();
        std::cout << "path " << path << std::endl;
        processor.knn.readData(path);
    }
    else if(writeText.hasButton(button))
    {
        std::cout << "write Text pressed" << std::endl;
        std::string path = writeText.getText().toStdString();
        std::cout << "path " << path << std::endl;
        processor.knn.writeTextData(path);
    }
    else if(readText.hasButton(button))
    {
        std::cout << "read Text pressed" << std::endl;
        std::string path = readText.getText().toStdString();
        std::cout << "path " << path << std::endl;
        processor.knn.readTextData(path);
    }
    else if(featureNumber.hasButton(button))
    {
        processor.featuresUsed = featureNumber.getText().getIntValue();
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

void DemoEditor::updateBFCCDataLabel(){
    std::string text = processor.bfcc.getInfoString();
    text += "\n\nFeatures used: ";
    text += std::to_string(processor.featuresUsed);
    this->bfccInfo.setText(text,NotificationType::dontSendNotification);
}

void DemoEditor::comboBoxChanged(ComboBox* cb)
{
    if(cb == &classifierState)
    {
        switch (cb->getSelectedId()) {
            case 1:
                processor.classifierState = DemoProcessor::CState::train;
                break;
            case 2:
                processor.classifierState = DemoProcessor::CState::classify;
                break;
            case 3:
                processor.classifierState = DemoProcessor::CState::idle;
                break;
        }
    }
}

void DemoEditor::updateKnnLabel(unsigned int match, float distance)
{
    this->dispTimbre.setText("Timbre: " + std::to_string(match), NotificationType::dontSendNotification);
    this->dispDist.setText("Distance: " + std::to_string(distance), NotificationType::dontSendNotification);
}
