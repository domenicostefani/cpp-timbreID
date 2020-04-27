/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - cepstrum Module

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
    titleLabel.setText("Real Cepstrum extractor (real portion of the IFT of log magnitude spectrum)", NotificationType::dontSendNotification);
    titleLabel.setFont (Font (18.0f, Font::bold));
    titleLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabel();

    addAndMakeVisible(resLabel);
    resLabel.setJustificationType(Justification::topLeft);
    std::vector<float> placeholder{0.0};
    updateResLabel(placeholder);

    addAndMakeVisible(computeBtn);
    computeBtn.setButtonText("Compute cepstrum");
    computeBtn.addListener(this);

    addAndMakeVisible(windowFuncBox);
    windowFuncBox.setLabelText("Window function [0-4]): ");
    windowFuncBox.setListener(this);

    addAndMakeVisible(spectrumTypeBox);
    spectrumTypeBox.setLabelText("Spectrum Type [0-1]): ");
    spectrumTypeBox.setListener(this);

    addAndMakeVisible(cepstrumTypeBox);
    cepstrumTypeBox.setLabelText("Cepstrum Type [0-1]): ");
    cepstrumTypeBox.setListener(this);

    addAndMakeVisible(spectrumOffsetBox);
    spectrumOffsetBox.setLabelText("spectrum Offset [0-1]): ");
    spectrumOffsetBox.setListener(this);

    addAndMakeVisible(windowSizeBox);
    windowSizeBox.setLabelText("Window Size(samples) ["+std::to_string(tIDLib::MINWINDOWSIZE)+"-inf]: ");
    windowSizeBox.setListener(this);

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

    Rectangle<int> bottom = usable.removeFromBottom(6.0 * boxHeight);
    Rectangle<int> data = usable.removeFromBottom(usable.getHeight()*0.6);
    Rectangle<int> top = usable;

    dataLabel.setBounds(data.removeFromLeft(data.getWidth()*0.5));
    resLabel.setBounds(data);

    titleLabel.setBounds(top.removeFromTop(top.getHeight()/4));
    computeBtn.setBounds(top.reduced(20));

    bottom.removeFromTop(boxHeight);
    windowFuncBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    spectrumTypeBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    cepstrumTypeBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    spectrumOffsetBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    windowSizeBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
}


void DemoEditor::buttonClicked (Button * button)
{
    if(button == &computeBtn)
    {
        std::vector<float> res = processor.cepstrum.compute();
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
            this->processor.cepstrum.setWindowFunction(selected);
        }

    }
    else if(spectrumTypeBox.hasButton(button))
    {
        unsigned int specType = spectrumTypeBox.getText().getIntValue();
        if(specType < 0 || specType > 1)
            spectrumTypeBox.showErr("Must be either 0 or 1");
        else
            this->processor.cepstrum.setSpectrumType(specType==0 ? tIDLib::SpectrumType::magnitudeSpectrum : tIDLib::SpectrumType::powerSpectrum);
    }
    else if(cepstrumTypeBox.hasButton(button))
    {
        unsigned int cepsType = cepstrumTypeBox.getText().getIntValue();
        if(cepsType < 0 || cepsType > 1)
            cepstrumTypeBox.showErr("Must be either 0 or 1");
        else
            this->processor.cepstrum.setCepstrumType(cepsType==0 ? tIDLib::CepstrumType::magnitudeCepstrum : tIDLib::CepstrumType::powerCepstrum);
    }
    else if(spectrumOffsetBox.hasButton(button))
    {
        unsigned int specOff = spectrumOffsetBox.getText().getIntValue();
        if(specOff < 0 || specOff > 1)
            spectrumOffsetBox.showErr("Must be either 0 or 1");
        else
            this->processor.cepstrum.setSpectrumOffset(specOff==1);
    }
    else if(windowSizeBox.hasButton(button))
    {
        unsigned int windowSize = windowSizeBox.getText().getIntValue();
        if(windowSize < tIDLib::MINWINDOWSIZE)
            windowSizeBox.showErr("Must be greater than "+std::to_string(tIDLib::MINWINDOWSIZE));
        else
            processor.cepstrum.setWindowSize(windowSize);
    }
}

void DemoEditor::updateDataLabel(){

    std::string text = this->processor.cepstrum.getInfoString();
    this->dataLabel.setText(text,NotificationType::dontSendNotification);
}

void DemoEditor::updateResLabel(std::vector<float> &coeff){
    std::string text = "Cepstrum:\n ";
    for(float &val: coeff)
        text += (std::to_string(val) + "  -  ");

    this->resLabel.setText(text,NotificationType::dontSendNotification);
}
