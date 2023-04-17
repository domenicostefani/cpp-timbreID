/*
 Box with a label, a text editor and a button.
 When the button can be pressed to set a parameter value read from the editor
*/

#pragma once

#include <JuceHeader.h>

class SetParamBox  : public Component
{
public:
    explicit SetParamBox ();
    ~SetParamBox() override;

    void paint (Graphics& g) override;
    void resized() override;

    void setLabelText(String text);

    void setListener(Button::Listener *listener);
    String getText();
    bool hasButton(Button* button);

    void showErr(std::string text);
protected:
    TextButton setButton;
    Label boxLabel;
    TextEditor inputText;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SetParamBox)
};

class setDualParamBox : public SetParamBox
{
public:
    setDualParamBox() : SetParamBox()
    {
        addAndMakeVisible(inputText2);
    }

    String getText1()
    {
        return this->inputText.getText();
    }

    String getText2()
    {
        return inputText2.getText();
    }

    void resized() override
    {
        Rectangle<int> area = getLocalBounds();
        boxLabel.setBounds(area.removeFromLeft(area.getWidth()*0.4));   //40% left
        inputText.setBounds(area.removeFromLeft(area.getWidth()*0.333)); //20% left of total
        inputText2.setBounds(area.removeFromLeft(area.getWidth()*0.5)); //20% left of total
        setButton.setBounds(area);  //remaining 20%
    }
private:
    TextEditor inputText2;
};
