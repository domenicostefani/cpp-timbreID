/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <iostream>


//==============================================================================
DemoEditor::DemoEditor (DemoProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (200, 200);

    addAndMakeVisible(button);
    button.setButtonText("Compute zero crossings");
    button.addListener(this);

    addAndMakeVisible(label);
    label.setJustificationType(Justification::centred);
    monitorSetBlockSize(0);
    monitorSetSampleRate(0);
    monitorSetCrossings(0);
}

DemoEditor::~DemoEditor()
{
}

//==============================================================================
void DemoEditor::paint (Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

//    g.setColour (Colours::white);
//    g.setFont (15.0f);
//    g.drawFittedText ("Ciao", getLocalBounds(), Justification::centred, 1);

    monitorSetSampleRate(processor.getSampleRate());
    monitorSetBlockSize(processor.getBlockSize());

}

void DemoEditor::resized()
{
    Rectangle<int> area = getLocalBounds();
    Rectangle<int> usable = area.reduced(10);

    Rectangle<int> bottom = usable.removeFromBottom(usable.getHeight()*0.5);
    Rectangle<int> top = usable;

    button.setBounds(bottom.reduced(10));
    label.setBounds(top);

}


void DemoEditor::buttonClicked (Button * button){
    if(button == &this->button)
    {
        bgColor = Colours::aliceblue;
        uint32 crossings = processor.computeZeroCrossing();
        std::cout << "crossings: " << crossings << std::endl;
        monitorSetCrossings(crossings);
    }
}

void DemoEditor::monitorSetSampleRate(double sampleRate){
    sampleRate_s = std::to_string((int)sampleRate);
    this->label.setText("Sample rate:  "+sampleRate_s+"\nBlock size:  "+blockSize_s+"\nCrossings:  " +crossings_s,NotificationType::dontSendNotification);
}

void DemoEditor::monitorSetBlockSize(int blockSize){
    blockSize_s = std::to_string(blockSize);
    this->label.setText("Sample rate:  "+sampleRate_s+"\nBlock size:  "+blockSize_s+"\nCrossings:  " +crossings_s,NotificationType::dontSendNotification);
}

void DemoEditor::monitorSetCrossings(uint32 crossings){
    crossings_s = std::to_string(crossings);
    this->label.setText("Sample rate:  "+sampleRate_s+"\nBlock size:  "+blockSize_s+"\nCrossings:  " +crossings_s,NotificationType::dontSendNotification);
}
