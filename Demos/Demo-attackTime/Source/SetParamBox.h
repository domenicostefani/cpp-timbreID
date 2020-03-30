/*
  ==============================================================================

    SetParamBox.h
    Created: 27 Mar 2020 2:52:49pm
    Author:  domenico

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/*
*/
class SetParamBox    : public Component
{
public:
    SetParamBox();
    ~SetParamBox();

    void paint (Graphics&) override;
    void resized() override;

    Label labelText;
    Label inputText;
    TextButton setButton;
    void addListener(Button::Listener* listener);

    template<typename ParameterType>
    void update(const std::string& parameterName, const ParameterType& currentValue)
    {
        labelText.setText("Set "+parameterName+" (current: "+std::to_string(currentValue)+"):",NotificationType::dontSendNotification);
    }

    void setFloat(bool value)
    {
        if(value)
        {
            inputText.onEditorShow = [this]
            {
                auto* editor = inputText.getCurrentTextEditor();

                auto editorFont = editor->getFont();
                editorFont.setItalic (true);
                editor->setFont (editorFont);
                editor->setInputRestrictions(15,"0123456789.");
            };
        } else {
            inputText.onEditorShow = [this]
            {
                auto* editor = inputText.getCurrentTextEditor();

                auto editorFont = editor->getFont();
                editorFont.setItalic (true);
                editor->setFont (editorFont);
                editor->setInputRestrictions(15,"0123456789");
            };
        }
    }

private:
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SetParamBox)
};
