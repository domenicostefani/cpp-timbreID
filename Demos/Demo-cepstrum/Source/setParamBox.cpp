#include "setParamBox.h"

//==============================================================================
SetParamBox::SetParamBox ()
{
    addAndMakeVisible(boxLabel);
    addAndMakeVisible(inputText);
    addAndMakeVisible(setButton);

    setButton.setButtonText("Set");
}

SetParamBox::~SetParamBox(){}

//==============================================================================
void SetParamBox::paint (Graphics& g)
{
    g.fillAll (Colour (0xff323e44));
}

void SetParamBox::resized()
{
    Rectangle<int> area = getLocalBounds();
    boxLabel.setBounds(area.removeFromLeft(area.getWidth()*0.4));   //40% left
    inputText.setBounds(area.removeFromLeft(area.getWidth()*0.667)); //40% left of total
    setButton.setBounds(area);  //remaining 20%
}

void SetParamBox::setLabelText(String text)
{
    boxLabel.setText(text, dontSendNotification);
}

void SetParamBox::setListener(Button::Listener *listener)
{
    setButton.addListener(listener);
}

String SetParamBox::getText()
{
    return inputText.getText();
}


bool SetParamBox::hasButton(Button* button)
{
    return (button == &this->setButton);
}

void SetParamBox::showErr(std::string text)
{
    inputText.setText(text,dontSendNotification);
}
