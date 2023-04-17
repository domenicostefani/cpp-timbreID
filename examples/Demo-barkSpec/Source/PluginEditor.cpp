/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - barkSpec Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 23rd April 2020

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
    setResizable(true,true);
    setSize (700, 600);

    addAndMakeVisible(titleLabel);
    titleLabel.setText("BarkSpec extractor", NotificationType::dontSendNotification);
    titleLabel.setFont (Font (20.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabel();

    addAndMakeVisible(resLabel);
    resLabel.setJustificationType(Justification::topLeft);
    std::vector<float> placeholder{0.0};
    updateResLabel(placeholder);

    addAndMakeVisible(computeBtn);
    computeBtn.setButtonText("Compute bark spec");
    computeBtn.addListener(this);


    addAndMakeVisible(windowFuncBox);
    windowFuncBox.setLabelText("Windows function [0-4]): ");
    windowFuncBox.setListener(this);

    addAndMakeVisible(filteringBox);
    filteringBox.setLabelText("Filter State [0-1]): ");
    filteringBox.setListener(this);

    addAndMakeVisible(filterOpBox);
    filterOpBox.setLabelText("Filter Operation [0-1]): ");
    filterOpBox.setListener(this);

    addAndMakeVisible(spectrumTypeBox);
    spectrumTypeBox.setLabelText("Spectrum Type [0-1]): ");
    spectrumTypeBox.setListener(this);

    addAndMakeVisible(windowSizeBox);
    windowSizeBox.setLabelText("Window Size(samples) ["+std::to_string(tIDLib::MINWINDOWSIZE)+"-inf]: ");
    windowSizeBox.setListener(this);

    addAndMakeVisible(normalizeBox);
    normalizeBox.setLabelText("Normalize Flag [0-1]): ");
    normalizeBox.setListener(this);

    addAndMakeVisible(createFilterbankBox);
    createFilterbankBox.setLabelText("CreateFilterbank. Spacing ["+std::to_string(tIDLib::MINBARKSPACING)+","+std::to_string(tIDLib::MAXBARKSPACING)+"]: ");
    createFilterbankBox.setListener(this);

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
    int boxHeight = 25;
    Rectangle<int> area = getLocalBounds();
    Rectangle<int> usable = area.reduced(10);

    Rectangle<int> bottom = usable.removeFromBottom(8.0 * boxHeight);
    Rectangle<int> data = usable.removeFromBottom(usable.getHeight()*0.8);
    Rectangle<int> top = usable;

    dataLabel.setBounds(data.removeFromLeft(data.getWidth()*0.5));
    resLabel.setBounds(data);

    titleLabel.setBounds(top.removeFromTop(top.getHeight()/4));
    computeBtn.setBounds(top.reduced(20));

    bottom.removeFromTop(boxHeight);
    windowFuncBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    filteringBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    filterOpBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    spectrumTypeBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    windowSizeBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    normalizeBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    createFilterbankBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
}


void DemoEditor::buttonClicked (Button * button)
{
    if(button == &computeBtn)
    {
        std::vector<float> res = processor.barkSpec.compute();
        updateResLabel(res);
    }
    else if(windowFuncBox.hasButton(button))
    {
        unsigned int windowFunc = windowFuncBox.getText().getIntValue();
        if(windowFunc < 0 || windowFunc > 4)
            windowFuncBox.showErr("Must be int in range 0-4");
        else
        {
            tIDLib::WindowFunctionType selected;
            switch(windowFunc)
            {
                case 0:
                    selected = tIDLib::WindowFunctionType::rectangular;
                    break;
                case 1:
                    selected = tIDLib::WindowFunctionType::blackman;
                    break;
                case 2:
                    selected = tIDLib::WindowFunctionType::cosine;
                    break;
                case 3:
                    selected = tIDLib::WindowFunctionType::hamming;
                    break;
                case 4:
                    selected = tIDLib::WindowFunctionType::hann;
                    break;
            }
            this->processor.barkSpec.setWindowFunction(selected);
        }

    }
    else if(filteringBox.hasButton(button))
    {
        unsigned int filterState = filteringBox.getText().getIntValue();
        if(filterState < 0 || filterState > 1)
            filteringBox.showErr("Must be either 0 or 1");
        else
            this->processor.barkSpec.setFiltering(filterState==0 ? tIDLib::FilterState::filterDisabled : tIDLib::FilterState::filterEnabled);
    }
    else if(filterOpBox.hasButton(button))
    {
        unsigned int filterOp = filterOpBox.getText().getIntValue();
        if(filterOp < 0 || filterOp > 1)
            filterOpBox.showErr("Must be either 0 or 1");
        else
            this->processor.barkSpec.setFilterOperation(filterOp==0 ? tIDLib::FilterOperation::sumFilterEnergy : tIDLib::FilterOperation::averageFilterEnergy);
    }
    else if(spectrumTypeBox.hasButton(button))
    {
        unsigned int specType = spectrumTypeBox.getText().getIntValue();
        if(specType < 0 || specType > 1)
            spectrumTypeBox.showErr("Must be either 0 or 1");
        else
            this->processor.barkSpec.setSpectrumType(specType==0 ? tIDLib::SpectrumType::magnitudeSpectrum : tIDLib::SpectrumType::powerSpectrum);
    }
    else if(windowSizeBox.hasButton(button))
    {
        unsigned int windowSize = windowSizeBox.getText().getIntValue();
        if(windowSize < tIDLib::MINWINDOWSIZE)
            windowSizeBox.showErr("Must be greater than "+std::to_string(tIDLib::MINWINDOWSIZE));
        else
            processor.barkSpec.setWindowSize(windowSize);
    }
    else if(normalizeBox.hasButton(button))
    {
        unsigned int norm = normalizeBox.getText().getIntValue();
        if(norm < 0 || norm > 1)
            normalizeBox.showErr("Must be either 0 or 1");
        else
            this->processor.barkSpec.setNormalize((bool)norm);
    }
    else if(createFilterbankBox.hasButton(button))
    {
        float spacing = createFilterbankBox.getText().getFloatValue();
        if(spacing < tIDLib::MINBARKSPACING || spacing > tIDLib::MAXBARKSPACING)
            createFilterbankBox.showErr("Must be float between "+std::to_string(tIDLib::MINBARKSPACING)+" and "+std::to_string(tIDLib::MAXBARKSPACING));
        else
            this->processor.barkSpec.createFilterbank(spacing);
    }
}

void DemoEditor::updateDataLabel(){

    std::string text = this->processor.barkSpec.getInfoString();
    this->dataLabel.setText(text,NotificationType::dontSendNotification);
}

void DemoEditor::updateResLabel(std::vector<float> &coeff){
    std::string text = "barkSpec:\n ";
    for(float &val: coeff)
        text += (std::to_string(val) + "  -  ");

    this->resLabel.setText(text,NotificationType::dontSendNotification);
}
