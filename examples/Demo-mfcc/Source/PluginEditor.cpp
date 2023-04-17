/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - mfcc Module (Mel Frequency Cepstral Coefficients)

  Mel-frequency cepstrum is much different than raw cepstrum.
  The most significant differences are an emphasis on lower spectral content and
  the use of a DCT rather than a FT in the final step of the process. When mfcc~
  receives a bang, it spits out the MFCCs for the most recent analysis window as
  a list. The default 100mel filterbank spacing produces a 38-component MFCC
  vector regardless of window size. MFCC components are normalized to be between
  1 and -1 by default.
  If normalized, the first MFCC will always have a value of 1

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 27th April 2020

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
    titleLabel.setText("MFCC extractor", NotificationType::dontSendNotification);
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
    computeBtn.setButtonText("Compute mfcc");
    computeBtn.addListener(this);

    addAndMakeVisible(windowFuncBox);
    windowFuncBox.setLabelText("Window function [0-4]): ");
    windowFuncBox.setListener(this);

    addAndMakeVisible(filterStateBox);
    filterStateBox.setLabelText("Filter State [0-1]): ");
    filterStateBox.setListener(this);

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
    normalizeBox.setLabelText("Normalize [0-1]): ");
    normalizeBox.setListener(this);

    addAndMakeVisible(createFilterbankBox);
    createFilterbankBox.setLabelText("CreateFilterbank. Spacing ["+std::to_string(tIDLib::MINMELSPACING)+","+std::to_string(tIDLib::MAXMELSPACING)+"]: ");
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
    Rectangle<int> data = usable.removeFromBottom(usable.getHeight()*0.6);
    Rectangle<int> top = usable;

    dataLabel.setBounds(data.removeFromLeft(data.getWidth()*0.5));
    resLabel.setBounds(data);

    titleLabel.setBounds(top.removeFromTop(top.getHeight()/4));
    computeBtn.setBounds(top.reduced(20));

    bottom.removeFromTop(boxHeight);


    windowFuncBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
    filterStateBox.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:bottom.getHeight()));
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
        std::vector<float> res = processor.mfcc.compute();
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
            this->processor.mfcc.setWindowFunction(selected);
        }

    }
    else if(filterStateBox.hasButton(button))
    {
        unsigned int filtState = filterStateBox.getText().getIntValue();
        if(filtState < 0 || filtState > 1)
            filterStateBox.showErr("Must be either 0 or 1");
        else
            this->processor.mfcc.setFiltering(filtState==0 ? tIDLib::FilterState::filterDisabled: tIDLib::FilterState::filterEnabled);
    }
    else if(filterOpBox.hasButton(button))
    {
        unsigned int filtOp = filterOpBox.getText().getIntValue();
        if(filtOp < 0 || filtOp > 1)
            filterOpBox.showErr("Must be either 0 or 1");
        else
            this->processor.mfcc.setFilterOperation(filtOp==0 ? tIDLib::FilterOperation::sumFilterEnergy : tIDLib::FilterOperation::averageFilterEnergy);
    }
    else if(spectrumTypeBox.hasButton(button))
    {
        unsigned int specType = spectrumTypeBox.getText().getIntValue();
        if(specType < 0 || specType > 1)
            spectrumTypeBox.showErr("Must be either 0 or 1");
        else
            this->processor.mfcc.setSpectrumType(specType==0 ? tIDLib::SpectrumType::magnitudeSpectrum : tIDLib::SpectrumType::powerSpectrum);
    }
    else if(windowSizeBox.hasButton(button))
    {
        unsigned int windowSize = windowSizeBox.getText().getIntValue();
        if(windowSize < tIDLib::MINWINDOWSIZE)
            windowSizeBox.showErr("Must be greater than "+std::to_string(tIDLib::MINWINDOWSIZE));
        else
        {
            try
            {
                processor.mfcc.setWindowSize(windowSize);
            }
            catch(std::exception &e)
            {
                windowSizeBox.showErr(e.what());
            }
        }
    }
    else if(normalizeBox.hasButton(button))
    {
        unsigned int norm = normalizeBox.getText().getIntValue();
        if(norm < 0 || norm > 1)
            normalizeBox.showErr("Must be either 0 or 1");
        else
            this->processor.mfcc.setNormalize(norm==1);
    }
    else if(createFilterbankBox.hasButton(button))
    {
        float spacing = createFilterbankBox.getText().getFloatValue();
        if(spacing < tIDLib::MINMELSPACING || spacing > tIDLib::MAXMELSPACING)
            createFilterbankBox.showErr("Must be float between "+std::to_string(tIDLib::MINMELSPACING)+" and "+std::to_string(tIDLib::MAXMELSPACING) + " mels");
        else
        {
            try
            {
                this->processor.mfcc.createFilterbank(spacing);
            }
            catch(std::exception &e)
            {
                createFilterbankBox.showErr(e.what());
            }
        }
    }
}

void DemoEditor::updateDataLabel(){

    std::string text = this->processor.mfcc.getInfoString();
    this->dataLabel.setText(text,NotificationType::dontSendNotification);
}

void DemoEditor::updateResLabel(std::vector<float> &coeff){
    std::string text = "mfcc:\n ";
    for(float &val: coeff)
        text += (std::to_string(val) + "  -  ");

    this->resLabel.setText(text,NotificationType::dontSendNotification);
}
