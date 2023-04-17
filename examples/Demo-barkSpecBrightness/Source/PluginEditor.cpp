/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - barkSpecBrightness Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 15th April 2020

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
    addAndMakeVisible(titleLabel);
    titleLabel.setText("BarkSpec Brighness extractor", NotificationType::dontSendNotification);
    titleLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabel();

    addAndMakeVisible(resLabel);
    resLabel.setJustificationType(Justification::topLeft);
    updateResLabel(0);

    addAndMakeVisible(computeBtn);
    computeBtn.setButtonText("Compute Brightness");
    computeBtn.addListener(this);

    addAndMakeVisible(windowSizeBox);
    windowSizeBox.setLabelText("Window Size(samples) [4-inf]: ");
    windowSizeBox.setListener(this);

    addAndMakeVisible(boundaryBox);
    boundaryBox.setLabelText("BarkBoundary): ");
    boundaryBox.setListener(this);


    pollingTimer.startPolling();
}

DemoEditor::~DemoEditor()
{
}

//==============================================================================
void DemoEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    updateDataLabel();
}

void DemoEditor::resized()
{
    Rectangle<int> area = getLocalBounds();
    Rectangle<int> usable = area.reduced(10);

    Rectangle<int> bottom = usable.removeFromBottom(usable.getHeight()*0.3);
    Rectangle<int> data = usable.removeFromBottom(usable.getHeight()*0.4);
    Rectangle<int> top = usable;

    dataLabel.setBounds(data.removeFromLeft(data.getWidth()*0.5));
    resLabel.setBounds(data);

    titleLabel.setBounds(top.removeFromTop(top.getHeight()/3));
    computeBtn.setBounds(top.reduced(20));

    int boxHeight = 25;
    windowSizeBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    boundaryBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
}


void DemoEditor::buttonClicked (Button * button)
{
    if(button == &computeBtn)
    {
        float res = processor.barkSpecBrightness.compute();
        updateResLabel(res);
    }
    else if(windowSizeBox.hasButton(button))
    {
         unsigned int windowSize = windowSizeBox.getText().getIntValue();
         std::cout << "Setting window size time to " << windowSize << " samples" << std::endl;

         processor.barkSpecBrightness.setWindowSize(windowSize);
    }
    else if(boundaryBox.hasButton(button))
    {
         float boundary = boundaryBox.getText().getFloatValue();
         std::cout << "Setting bark boundary to " << boundary << std::endl;

         processor.barkSpecBrightness.setBoundary(boundary);
    }
}

void DemoEditor::updateDataLabel(){
    std::string sampleRate_s = std::to_string((int)this->processor.getSampleRate());
    std::string blockSize_s = std::to_string((int)this->processor.getBlockSize());
    std::string windowSize_s = std::to_string((int)this->processor.barkSpecBrightness.getWindowSize());

    std::string text = "Sample rate:  "+sampleRate_s+"\nBlock size:  "+blockSize_s+"\nWindow size:  " +windowSize_s;

    text += "\nBarkBoundary: ";
    float barkBoundary = processor.barkSpecBrightness.getBoundary();
    text += std::to_string(barkBoundary);

    this->dataLabel.setText(text,NotificationType::dontSendNotification);
}

void DemoEditor::updateResLabel(float brightness){
    std::string text = "Brightness: ";
    text += std::to_string(brightness);
    this->resLabel.setText(text,NotificationType::dontSendNotification);
}
