/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - Feature Extractor Plugin

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 10th September 2020

  ==============================================================================
 */

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <iostream>
#include <string>

#define COMPACT_INTERFACE

//==============================================================================
std::string generateName(std::string directoryPath)
{
    std::string filename = "test.csv";

    File tmpfile = File(directoryPath+filename);
    int counter = 1;
    while(tmpfile.exists())
    {
        filename = "test" + std::to_string(counter++) + ".csv";
        tmpfile = File(directoryPath+filename);
    }
    return filename;
}


DemoEditor::DemoEditor (DemoProcessor& p)
    : AudioProcessorEditor (&p), processor (p)
{

   #ifndef COMPACT_INTERFACE
    setSize (1200, 750);
   #else
    setSize (500, 200);
   #endif

    addAndMakeVisible(titleLabel);

    std::string specs =  std::to_string(processor.samplesPerBlock)+"@"+std::to_string(int(processor.sampleRate));

    std::string date =  "(2023-01-18)";
    std::string feature_window_size = std::to_string(DEFINED_WINDOW_SIZE) + " ("+std::to_string(DEFINED_WINDOW_SIZE/processor.sampleRate*1000)+"ms)";
   #ifdef WINDOWED_FEATURE_EXTRACTORS
    specs = "Windowed " + specs;
   #else
    specs = "Singlewindow! " + specs;
   #endif
    

    std::string titletext = specs + "\nOnset detector + 8 Feature Extractors "+feature_window_size+" "+date;


    titleLabel.setText(titletext, NotificationType::dontSendNotification);
    titleLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabels();

    addAndMakeVisible(onsetLed);

    // addAndMakeVisible(boxIoI);
    // boxIoI.setLabelText("Min Inter Onset Interval (millis) [0-inf]: ");
    // boxIoI.addListener(this);

    // addAndMakeVisible(boxThresh);
    // boxThresh.setLabelText("Onset Threshold: ");
    // boxThresh.addListener(this);

    // addAndMakeVisible(boxSilenceThresh);
    // boxSilenceThresh.setLabelText("Silence Threshold: ");
    // boxSilenceThresh.addListener(this);

    pollingTimer.startPolling();

    for(Label& extractor : extractorData)
    {
        addAndMakeVisible(extractor);
        extractor.setJustificationType(Justification::topLeft);
    }

    addAndMakeVisible(storageTitle);
    storageTitle.setText("Storage Section", NotificationType::dontSendNotification);
    storageTitle.setJustificationType(Justification::centred);

    addAndMakeVisible(storageStateBox);
    storageStateBox.addItem("Store",1);
    storageStateBox.addItem("Idle",2);
    auto val = processor.storageState.load();
    int selectedid = -1;
    if(val == DemoProcessor::StorageState::idle)
        selectedid = 2;
    else if(val == DemoProcessor::StorageState::store)
        selectedid = 1;
    else
        throw std::logic_error("val is equal to DemoProcessor::StorageState::idle: "+std::to_string(val == DemoProcessor::StorageState::idle));

    storageStateBox.setSelectedId(selectedid);

    storageStateBox.setJustificationType(Justification::centred);
    // storageStateBox.addListener(this);
    storestateAttachment.reset(new ComboBoxAttachment(processor.parameters,processor.STORESTATE_ID,storageStateBox));

    addAndMakeVisible(clearAll);  //TextButton
    // clearAll.addListener(this);
    clearAll.setButtonText("clear");
    clearAll.setClickingTogglesState (true);
    clearAttachment.reset(new ButtonAttachment(processor.parameters,processor.CLEAR_ID,clearAll));

    addAndMakeVisible(write);  //SetParamBox
    // write.addListener(this);
    write.setLabelText("write");
    this->dirpath = "~/DatasetFeatureExtraction/";
    write.setDefaultText1(dirpath);
    write.setDefaultText2(generateName(this->dirpath));

    write.getButtonReference().setClickingTogglesState(true);
    savefileAttachment.reset(new ButtonAttachment(processor.parameters,processor.SAVEFILE_ID,write.getButtonReference()));

    addAndMakeVisible(dispInfo);  //Label
    dispInfo.setText("dispInfo", NotificationType::dontSendNotification);
}

DemoEditor::~DemoEditor()
{
}

//==============================================================================
void DemoEditor::paint (Graphics& g)
{
   #ifdef WINDOWED_FEATURE_EXTRACTORS
    g.fillAll (juce::Colour (0xffff7080));
   #else
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
   #endif

    this->mSampleRate = processor.getSampleRate();
    this->mBlockSize = processor.getBlockSize();
    updateDataLabels();

    int v = storageStateBox.getSelectedId(); // TODO: remove
    NULL;
}

void DemoEditor::resized()
{
    int boxHeight = 25;
    Rectangle<int> area = getLocalBounds();
    Rectangle<int> usable = area.reduced(10);

    titleLabel.setBounds(usable.removeFromTop(boxHeight*2));


   #ifndef COMPACT_INTERFACE
    Rectangle<int> data = usable.removeFromTop(usable.getHeight()*0.4);
    Rectangle<int> bottom = usable;
   #else
    Rectangle<int> data = usable;
   #endif

    onsetLed.setBounds(data.removeFromLeft(70));
   #ifndef COMPACT_INTERFACE
    Rectangle<int> onsetdata = data.removeFromLeft(data.getWidth()*0.5);
    dataLabel.setBounds(onsetdata.removeFromTop(onsetdata.getHeight()*0.5));
    boxIoI.setBounds(onsetdata.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxIoI.getHeight()));
    boxThresh.setBounds(onsetdata.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxThresh.getHeight()));
    boxSilenceThresh.setBounds(onsetdata.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxSilenceThresh.getHeight()));
   #endif
    Rectangle<int> classifierBox = data;

    // Knn classifier (timbreID on the right)
   #ifndef COMPACT_INTERFACE
    storageTitle.setBounds(classifierBox.removeFromTop(boxHeight*2));
   #endif
    Rectangle<int> storageCommands = classifierBox.removeFromTop(classifierBox.getHeight()*0.8);
    Rectangle<int> knnDisplay = classifierBox;

    storageStateBox.setBounds(storageCommands.removeFromTop(boxHeight));
    clearAll.setBounds(storageCommands.removeFromTop(boxHeight));
    storageCommands.removeFromTop(boxHeight*0.5);
    write.setBounds(storageCommands.removeFromTop(boxHeight));

    dispInfo.setBounds(knnDisplay.removeFromTop(boxHeight));
    //bottom
   #ifndef COMPACT_INTERFACE
    int dataWidth = bottom.getWidth()/EXTRACTOR_DATA_SIZE;
    for(int i = 0; i < EXTRACTOR_DATA_SIZE; ++i)
        extractorData[i].setBounds(bottom.removeFromLeft(dataWidth));
   #endif

}

std::vector<std::string> splitString(const std::string& s, char seperator)
{
   std::vector<std::string> output;

    std::string::size_type prev_pos = 0, pos = 0;

    while((pos = s.find(seperator, pos)) != std::string::npos)
    {
        std::string substring( s.substr(prev_pos, pos-prev_pos) );

        output.push_back(substring);

        prev_pos = ++pos;
    }

    output.push_back(s.substr(prev_pos, pos-prev_pos)); // Last word

    return output;
}

void DemoEditor::performStoreClearActions()
{

    if(processor.clear.load())
    {
        std::cout << "clearAll pressed" << std::endl;
        this->processor.csvsaver.clearEntries();
        this->processor.onsetCounterAtomic.store(0);
        processor.clear.store(false);
    }
    
    if(processor.savefile.load())
    {
        std::cout << "write pressed" << std::endl;
        File temp = File(write.getText1()+write.getText2());
        std::string path = temp.getFullPathName().toStdString();
        std::cout << "path " << path << std::endl;

        this->processor.csvsaver.writeToFile(path,processor.header);
        write.setDefaultText2(generateName(this->dirpath));
        processor.savefile.store(false);
    }
}

// void DemoEditor::buttonClicked (Button * button)
// {
//     if(boxIoI.hasButton(button))
//     {
//         unsigned int millis = boxIoI.getText().getIntValue();
//         std::cout << "Setting minIOI time to " << (millis/1000.0f) << " seconds at next memory release/alloc" << std::endl;
//         this->processor.aubioOnset.setOnsetMinioi(millis/1000.0f);
//     }
//     if(boxThresh.hasButton(button))
//     {
//         float val = boxThresh.getText().getFloatValue();
//         std::cout << "Setting Onset Threshold to " << val << " at next memory release/alloc" << std::endl;
//         this->processor.aubioOnset.setOnsetThreshold(val);
//     }
//     if(boxSilenceThresh.hasButton(button))
//     {
//         float val = boxSilenceThresh.getText().getFloatValue();
//         std::cout << "Setting Silence Threshold to " << val << " at next memory release/alloc" << std::endl;
//         this->processor.aubioOnset.setSilenceThreshold(val);
//     }
//     else if(&clearAll == button)
//     {
//         std::cout << "clearAll pressed" << std::endl;
//         this->processor.csvsaver.clearEntries();
//         this->processor.onsetCounterAtomic.store(0);
//     }
//     else if(write.hasButton(button))
//     {
//         std::cout << "write pressed" << std::endl;
//         File temp = File(write.getText1()+write.getText2());
//         std::string path = temp.getFullPathName().toStdString();
//         std::cout << "path " << path << std::endl;

//         this->processor.csvsaver.writeToFile(path,processor.header);
//         write.setDefaultText2(generateName(this->dirpath));
//     }
// }

void DemoEditor::updateDataLabels(){
    std::string text = "";
    text += "Using AUBIO ONSET DETECTOR";
    text+=("\nWindow Size: " + std::to_string(processor.aubioOnset.getWindowSize()));
    text+=("\nHopSize: " + std::to_string(processor.aubioOnset.getHopSize()));
    text+=("\nSilence Threshold: " + std::to_string(processor.aubioOnset.getSilenceThreshold()));
    text+=("\nOnset Method: " + processor.aubioOnset.getStringOnsetMethod());
    text+=("\nOnset Threshold: " + std::to_string(processor.aubioOnset.getOnsetThreshold()));
    text+=("\nOnset Min Inter-Onset Interval: " + std::to_string(processor.aubioOnset.getOnsetMinInterOnsetInterval()));
    std::string awstate = processor.aubioOnset.getAdaptiveWhitening() ? "enabled" : "disabled";
    text+=("\nAdaptive Whitening: " + awstate);

    this->dataLabel.setText(text,NotificationType::dontSendNotification);

    this->extractorData[0].setText("attackTime\n" +         processor.featexts.getInfoString(WFE::ATTACKTIME),         NotificationType::dontSendNotification);
    this->extractorData[1].setText("barkSpecBrightness\n" + processor.featexts.getInfoString(WFE::BARKSPECBRIGHTNESS), NotificationType::dontSendNotification);
    this->extractorData[2].setText("barkSpec\n" +           processor.featexts.getInfoString(WFE::BARKSPEC),           NotificationType::dontSendNotification);
    this->extractorData[3].setText("bfcc\n" +               processor.featexts.getInfoString(WFE::BFCC),               NotificationType::dontSendNotification);
    this->extractorData[4].setText("cepstrum\n" +           processor.featexts.getInfoString(WFE::CEPSTRUM),           NotificationType::dontSendNotification);
    this->extractorData[5].setText("mfcc\n" +               processor.featexts.getInfoString(WFE::MFCC),               NotificationType::dontSendNotification);
    this->extractorData[6].setText("peakSample\n" +         processor.featexts.getInfoString(WFE::PEAKSAMPLE),         NotificationType::dontSendNotification);
    this->extractorData[7].setText("zeroCrossing\n" +       processor.featexts.getInfoString(WFE::ZEROCROSSING),       NotificationType::dontSendNotification);

}

/**
 * Activate storage module when box entry is selected
 */
void DemoEditor::comboBoxChanged(ComboBox* cb)
{
    if(cb == &storageStateBox)
    {
        switch (cb->getSelectedId()) {
            case 1:
                processor.storageState.store(DemoProcessor::StorageState::store);
                break;
            case 2:
                processor.storageState.store(DemoProcessor::StorageState::idle);
                break;
        }
    }
}

/**
 * 
 */
void DemoEditor::updateOnsetCounter(unsigned int onsetCounter)
{
    this->dispInfo.setText("Instances: " + std::to_string(onsetCounter), NotificationType::dontSendNotification);
}
