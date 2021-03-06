/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - Feature Extractor Plugin

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 10th September 2020

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
    setSize (1200, 750);

    addAndMakeVisible(titleLabel);
    titleLabel.setText("Onset detector + 8 Feature Extractors + classifier", NotificationType::dontSendNotification);
    titleLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabels();

    addAndMakeVisible(onsetLed);

    addAndMakeVisible(boxIoI);
    boxIoI.setLabelText("Min Inter Onset Interval (millis) [0-inf]: ");
    boxIoI.addListener(this);

    addAndMakeVisible(boxThresh);
    boxThresh.setLabelText("Onset Threshold: ");
    boxThresh.addListener(this);

    addAndMakeVisible(boxSilenceThresh);
    boxSilenceThresh.setLabelText("Silence Threshold: ");
    boxSilenceThresh.addListener(this);

    pollingTimer.startPolling();

    for(Label& extractor : extractorData)
    {
        addAndMakeVisible(extractor);
        extractor.setJustificationType(Justification::topLeft);
    }

    addAndMakeVisible(storageTitle);
    storageTitle.setText("Storage Section", NotificationType::dontSendNotification);
    storageTitle.setJustificationType(Justification::centred);

    addAndMakeVisible(storageStateBox);
    storageStateBox.addItem("Idle",2);
    storageStateBox.addItem("Store",1);
    storageStateBox.setSelectedId(2);
    storageStateBox.setJustificationType(Justification::centred);
    storageStateBox.addListener(this);

    addAndMakeVisible(clearAll);  //TextButton
    clearAll.addListener(this);
    clearAll.setButtonText("clear");

    addAndMakeVisible(write);  //SetParamBox
    write.addListener(this);
    write.setLabelText("write");
    write.setDefaultText("./data/feature-db.csv");

    addAndMakeVisible(dispInfo);  //Label
    dispInfo.setText("dispInfo", NotificationType::dontSendNotification);
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
    updateDataLabels();
}

void DemoEditor::resized()
{
    int boxHeight = 25;
    Rectangle<int> area = getLocalBounds();
    Rectangle<int> usable = area.reduced(10);

    titleLabel.setBounds(usable.removeFromTop(boxHeight*2));
    Rectangle<int> data = usable.removeFromTop(usable.getHeight()*0.4);
    Rectangle<int> bottom = usable;

    onsetLed.setBounds(data.removeFromLeft(100));
    Rectangle<int> onsetdata = data.removeFromLeft(data.getWidth()*0.7);
    dataLabel.setBounds(onsetdata.removeFromTop(onsetdata.getHeight()*0.5));
    Rectangle<int> classifierBox = data;

    boxIoI.setBounds(onsetdata.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxIoI.getHeight()));
    boxThresh.setBounds(onsetdata.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxThresh.getHeight()));
    boxSilenceThresh.setBounds(onsetdata.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxSilenceThresh.getHeight()));

    // Knn classifier (timbreID on the right)
    storageTitle.setBounds(classifierBox.removeFromTop(boxHeight*2));
    Rectangle<int> storageCommands = classifierBox.removeFromTop(classifierBox.getHeight()*0.8);
    Rectangle<int> knnDisplay = classifierBox;

    storageStateBox.setBounds(storageCommands.removeFromTop(boxHeight));
    clearAll.setBounds(storageCommands.removeFromTop(boxHeight));
    storageCommands.removeFromTop(boxHeight*0.5);
    write.setBounds(storageCommands.removeFromTop(boxHeight));

    dispInfo.setBounds(knnDisplay.removeFromTop(boxHeight));

    //bottom
    int dataWidth = bottom.getWidth()/EXTRACTOR_DATA_SIZE;
    for(int i = 0; i < EXTRACTOR_DATA_SIZE; ++i)
        extractorData[i].setBounds(bottom.removeFromLeft(dataWidth));

}

std::vector<std::string> splitString(const std::string& s, char seperator)
{
   std::vector<std::string> output;

    std::string::size_type prev_pos = 0, pos = 0;

    while((pos = s.find(seperator, pos)) != std::string::npos)
    {
        std::string substring( s.substr(prev_pos, pos-prev_pos) );

        output.push_back(substring);

        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word

    return output;
}


void DemoEditor::buttonClicked (Button * button)
{
    if(boxIoI.hasButton(button))
    {
        unsigned int millis = boxIoI.getText().getIntValue();
        std::cout << "Setting minIOI time to " << (millis/1000.0f) << " seconds at next memory release/alloc" << std::endl;
        this->processor.aubioOnset.setOnsetMinioi(millis/1000.0f);
    }
    if(boxThresh.hasButton(button))
    {
        float val = boxThresh.getText().getFloatValue();
        std::cout << "Setting Onset Threshold to " << val << " at next memory release/alloc" << std::endl;
        this->processor.aubioOnset.setOnsetThreshold(val);
    }
    if(boxSilenceThresh.hasButton(button))
    {
        float val = boxSilenceThresh.getText().getFloatValue();
        std::cout << "Setting Silence Threshold to " << val << " at next memory release/alloc" << std::endl;
        this->processor.aubioOnset.setSilenceThreshold(val);
    }
    else if(&clearAll == button)
    {
        std::cout << "clearAll pressed" << std::endl;
        this->processor.csvsaver.clearEntries();
        this->processor.onsetCounterAtomic.store(0);
    }
    else if(write.hasButton(button))
    {
        std::cout << "write pressed" << std::endl;
        std::string path = write.getText().toStdString();
        std::cout << "path " << path << std::endl;
        this->processor.csvsaver.writeToFile(path,processor.header);
    }
}

void DemoEditor::updateDataLabels(){
    std::string text = "";
    text += "Using AUBIO ONSET DETECTOR";
    text+=("\nWindow Size: " + std::to_string(processor.aubioOnset.getWindowSize()));
    text+=("\nHopSize: " + std::to_string(processor.aubioOnset.getHopSize()));
    text+=("\nSilence Threshold: " + std::to_string(processor.aubioOnset.getSilenceThreshold()));
    text+=("\nOnset Method: " + processor.aubioOnset.getStringOnsetMethod());
    text+=("\nOnset Threshold: " + std::to_string(processor.aubioOnset.getOnsetThreshold()));
    text+=("\nOnset Min Inter-Onset Interval: " + std::to_string(processor.aubioOnset.getOnsetMinInterOnsetInterval()));
    std::string awstate = processor.aubioOnset.getAdaptiveWhitening() ? "enabled" : "disabled";
    text+=("\nAdaptive Whitening: " + awstate);

    this->dataLabel.setText(text,NotificationType::dontSendNotification);

    this->extractorData[0].setText("attackTime\n" + processor.attackTime.getInfoString(),NotificationType::dontSendNotification);
    this->extractorData[1].setText("barkSpecBrightness\n" + processor.barkSpecBrightness.getInfoString(),NotificationType::dontSendNotification);
    this->extractorData[2].setText("barkSpec\n" + processor.barkSpec.getInfoString(),NotificationType::dontSendNotification);
    this->extractorData[3].setText("bfcc\n" + processor.bfcc.getInfoString(),NotificationType::dontSendNotification);
    this->extractorData[4].setText("cepstrum\n" + processor.cepstrum.getInfoString(),NotificationType::dontSendNotification);
    this->extractorData[5].setText("mfcc\n" + processor.mfcc.getInfoString(),NotificationType::dontSendNotification);
    this->extractorData[6].setText("peakSample\n" + processor.peakSample.getInfoString(),NotificationType::dontSendNotification);
    this->extractorData[7].setText("zeroCrossing\n" + processor.zeroCrossing.getInfoString(),NotificationType::dontSendNotification);

}

/**
 * Activate storage module when box entry is selected
 */
void DemoEditor::comboBoxChanged(ComboBox* cb)
{
    if(cb == &storageStateBox)
    {
        switch (cb->getSelectedId()) {
            case 1:
                processor.storageState.store(DemoProcessor::StorageState::store);
                break;
            case 2:
                processor.storageState.store(DemoProcessor::StorageState::idle);
                break;
        }
    }
}

/**
 * 
 */
void DemoEditor::updateOnsetCounter(unsigned int onsetCounter)
{
    this->dispInfo.setText("Instances: " + std::to_string(onsetCounter), NotificationType::dontSendNotification);
}
