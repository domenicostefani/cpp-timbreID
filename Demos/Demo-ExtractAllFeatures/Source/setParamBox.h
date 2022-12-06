/*
 Box with a label, a text editor and a button.
 When the button can be pressed to set a parameter value read from the editor
*/

#pragma once

#include <JuceHeader.h>

class setParamBox  : public Component
{
public:
    explicit setParamBox(String buttonText = "Set");
    ~setParamBox() override;

    void paint (Graphics& g) override;
    void resized() override;

    void setLabelText(String text);
    void setButtonText(String text);

    void addListener(Button::Listener *listener);
    String getText();
    void setDefaultText(String text);
    bool hasButton(Button* button);

    
    TextButton& getButtonReference() { return this->setButton; }

protected:
    TextButton setButton;
    Label boxLabel;
    TextEditor inputText;
private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (setParamBox)
};

class setDualParamBox : public setParamBox
{
public:
    setDualParamBox(String buttonText = "Set") : setParamBox(buttonText)
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
        boxLabel.setBounds(area.removeFromLeft(area.getWidth()*0.2));
        inputText.setBounds(area.removeFromLeft(area.getWidth()*0.333));
        inputText2.setBounds(area.removeFromLeft(area.getWidth()*0.5));
        setButton.setBounds(area);  //remaining 20%
    }

    
    void setDefaultText(String text) = delete;

    void setDefaultText1(String text)
    {
        inputText.setText(text);
    }
    
    void setDefaultText2(String text)
    {
        inputText2.setText(text);
    }
private:
    TextEditor inputText2;
};
