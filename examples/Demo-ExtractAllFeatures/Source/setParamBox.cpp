#include "setParamBox.h"

//==============================================================================
setParamBox::setParamBox (String buttonText)
{
    addAndMakeVisible(boxLabel);
    addAndMakeVisible(inputText);
    addAndMakeVisible(setButton);
    
    setButton.setButtonText(buttonText);
}

setParamBox::~setParamBox(){}

//==============================================================================
void setParamBox::paint (Graphics& g)
{
    g.fillAll (Colour (0xff323e44));
}

void setParamBox::resized()
{
    Rectangle<int> area = getLocalBounds();
    boxLabel.setBounds(area.removeFromLeft(area.getWidth()*0.4));   //40% left
    inputText.setBounds(area.removeFromLeft(area.getWidth()*0.667)); //40% left of total
    setButton.setBounds(area);  //remaining 20%
}

void setParamBox::setLabelText(String text)
{
    boxLabel.setText(text, dontSendNotification);
}

void setParamBox::setButtonText(String text)
{
    setButton.setButtonText(text);
}

void setParamBox::addListener(Button::Listener *listener)
{
    setButton.addListener(listener);
}

String setParamBox::getText()
{
    return inputText.getText();
}

void setParamBox::setDefaultText(String text)
{
    inputText.setText(text);
}

bool setParamBox::hasButton(Button* button)
{
    return (button == &this->setButton);
}
