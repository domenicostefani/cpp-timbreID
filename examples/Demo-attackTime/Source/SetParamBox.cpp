/*
  ==============================================================================

    SetParamBox.cpp
    Created: 27 Mar 2020 2:52:49pm
    Author:  domenico

  ==============================================================================
*/

#include <JuceHeader.h>
#include "SetParamBox.h"

//==============================================================================
SetParamBox::SetParamBox()
{
    // In your constructor, you should add any child components, and
    // initialise any special settings that your component needs.
    addAndMakeVisible (inputText);
    inputText.setEditable (true);
    inputText.setColour (Label::backgroundColourId, Colour(0xff333333));
    inputText.setColour (Label::textColourId,Colour(0xfffafafa));
    inputText.setColour (Label::outlineWhenEditingColourId, Colour(0xfffafafa));

    inputText.onEditorShow = [this]
    {
        auto* editor = inputText.getCurrentTextEditor();

        auto editorFont = editor->getFont();
        editorFont.setItalic (true);
        editor->setFont (editorFont);
        editor->setInputRestrictions(15,"0123456789");
    };

    addAndMakeVisible (setButton);
    setButton.setButtonText("Set");

    addAndMakeVisible(labelText);
}

SetParamBox::~SetParamBox()
{
}

void SetParamBox::paint (Graphics& g)
{
}

void SetParamBox::resized()
{
    Rectangle<int> area = getLocalBounds();
    area = area.reduced(10);

    labelText.setBounds(area.removeFromLeft(area.getWidth()*0.5));

    inputText.setBounds(area.removeFromLeft(area.getWidth()*0.5));
    setButton.setBounds(area);

}

void SetParamBox::addListener(Button::Listener* listener)
{
    setButton.addListener(listener);
}








