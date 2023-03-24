/*
  ==============================================================================

  Plugin Editor

  DEMO PROJECT - TimbreID - Training + classification with knn

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 26th March 2020

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
    setSize (700, 750);
    addAndMakeVisible(titleLabel);
    titleLabel.setText("Onset detector + bfcc + classifier", NotificationType::dontSendNotification);
    titleLabel.setJustificationType(Justification::centred);

    addAndMakeVisible(dataLabel);
    dataLabel.setJustificationType(Justification::topLeft);
    updateDataLabel();

    addAndMakeVisible(onsetLed);

   #ifdef USE_AUBIO_ONSET

    addAndMakeVisible(boxIoI);
    boxIoI.setLabelText("Min Inter Onset Interval (millis) [0-inf]: ");
    boxIoI.addListener(this);

    addAndMakeVisible(boxThresh);
    boxThresh.setLabelText("Onset Threshold: ");
    boxThresh.addListener(this);

    addAndMakeVisible(boxSilenceThresh);
    boxSilenceThresh.setLabelText("Silence Threshold: ");
    boxSilenceThresh.addListener(this);
   #else
    addAndMakeVisible(boxDebounce);
    boxDebounce.setLabelText("Debounce time (millis) [0-inf]: ");
    boxDebounce.addListener(this);

    addAndMakeVisible(boxThresh);
    boxThresh.setLabelText("Thresholds (low,high): ");
    boxThresh.addListener(this);
   #endif


    pollingTimer.startPolling();


    addAndMakeVisible(bfccTitle);
    bfccTitle.setText("BFCC extractor", NotificationType::dontSendNotification);

    addAndMakeVisible(featureNumber);
    featureNumber.setLabelText("# Features Used:");
    featureNumber.addListener(this);

    addAndMakeVisible(bfccInfo);
    bfccInfo.setJustificationType(Justification::topLeft);






    addAndMakeVisible(classifierTitle);
    classifierTitle.setText("Classifier Section", NotificationType::dontSendNotification);
    classifierTitle.setJustificationType(Justification::centred);

    addAndMakeVisible(classifierState);
    classifierState.addItem("Idle",3);
    classifierState.addItem("Train",1);
    classifierState.addItem("Classify",2);
    classifierState.setSelectedId(3);
    classifierState.setJustificationType(Justification::centred);
    classifierState.addListener(this);


    addAndMakeVisible(uncluster);  //TextButton
    uncluster.addListener(this);
    uncluster.setButtonText("uncluster");

    addAndMakeVisible(clearAll);  //TextButton
    clearAll.addListener(this);
    clearAll.setButtonText("clear");

    addAndMakeVisible(cluster);  //SetParamBox
    cluster.addListener(this);
    cluster.setLabelText("cluster");

    addAndMakeVisible(write);  //SetParamBox
    write.addListener(this);
    write.setLabelText("write");
    // write.setDefaultText("./data/feature-db.timid");

    addAndMakeVisible(read);  //SetParamBox
    read.addListener(this);
    read.setLabelText("read");
    // read.setDefaultText("./data/feature-db.timid");

    addAndMakeVisible(writeText);  //SetParamBox
    writeText.addListener(this);
    writeText.setLabelText("write Text");
    // writeText.setDefaultText("./data/feature-db.csv");

    addAndMakeVisible(readText);  //SetParamBox
    readText.addListener(this);
    readText.setLabelText("read Text");
    // readText.setDefaultText("./data/feature-db.csv");

    addAndMakeVisible(manualCluster);  //SetParamBox
    manualCluster.addListener(this);
    manualCluster.setLabelText("ManCluster");
    // manualCluster.setDefaultText("0,20,40,60,80");


    addAndMakeVisible(dispTimbre);  //Label
    dispTimbre.setText("dispTimbre", NotificationType::dontSendNotification);

    addAndMakeVisible(dispDist);  //Label
    dispDist.setText("dispDist", NotificationType::dontSendNotification);
}

DemoEditor::~DemoEditor()
{
}

//==============================================================================
void DemoEditor::paint (Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));

    this->mSampleRate = processor.getSampleRate();
    this->mBlockSize = processor.getBlockSize();
    updateDataLabel();
}

void DemoEditor::resized()
{
    int boxHeight = 25;
    Rectangle<int> area = getLocalBounds();
    Rectangle<int> usable = area.reduced(10);

    titleLabel.setBounds(usable.removeFromTop(boxHeight*2));
    Rectangle<int> data = usable.removeFromTop(usable.getHeight()*0.3);
    Rectangle<int> bottom = usable;

    dataLabel.setBounds(data.removeFromLeft(data.getWidth()*0.5));
    onsetLed.setBounds(data.reduced(15));

   #ifdef USE_AUBIO_ONSET
    boxIoI.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxIoI.getHeight()));
    boxThresh.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxThresh.getHeight()));
    boxSilenceThresh.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxSilenceThresh.getHeight()));
    bottom.removeFromTop(boxHeight*1.5);
   #else
    boxDebounce.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxDebounce.getHeight()));
    boxThresh.setBounds(bottom.removeFromTop(boxHeight<bottom.getHeight()?boxHeight:boxDebounce.getHeight()));
    bottom.removeFromTop(boxHeight*1.5);
   #endif

    int spacer = 25;
    int lateralWidth = (bottom.getWidth()-spacer)/2;
    Rectangle<int> bottomLeft = bottom.removeFromLeft(lateralWidth);
    Rectangle<int> bottomRight = bottom.removeFromRight(lateralWidth);

    // Bfcc module on the topLeft
    bfccTitle.setBounds(bottomLeft.removeFromTop(boxHeight*2));
    featureNumber.setBounds(bottomLeft.removeFromTop(boxHeight));
    bfccInfo.setBounds(bottomLeft);

    // Knn classifier (timbreID on the right)
    classifierTitle.setBounds(bottomRight.removeFromTop(boxHeight*2));
    Rectangle<int> knnCommands = bottomRight.removeFromTop(bottomRight.getHeight()*0.8);
    Rectangle<int> knnDisplay = bottomRight;

    classifierState.setBounds(knnCommands.removeFromTop(boxHeight));
    cluster.setBounds(knnCommands.removeFromTop(boxHeight));
    manualCluster.setBounds(knnCommands.removeFromTop(boxHeight));
    uncluster.setBounds(knnCommands.removeFromTop(boxHeight));
    clearAll.setBounds(knnCommands.removeFromTop(boxHeight));
    knnCommands.removeFromTop(boxHeight*0.5);
    write.setBounds(knnCommands.removeFromTop(boxHeight));
    read.setBounds(knnCommands.removeFromTop(boxHeight));

    writeText.setBounds(knnCommands.removeFromTop(boxHeight));
    readText.setBounds(knnCommands.removeFromTop(boxHeight));

    dispTimbre.setBounds(knnDisplay.removeFromTop(boxHeight));
    dispDist.setBounds(knnDisplay.removeFromTop(boxHeight));
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


void DemoEditor::buttonClicked (Button * button)
{
   #ifdef USE_AUBIO_ONSET
    if(boxIoI.hasButton(button))
    {
        unsigned int millis = boxIoI.getText().getIntValue();
        std::cout << "Setting minIOI time to " << (millis/1000.0f) << " seconds at next memory release/alloc" << std::endl;
        this->processor.aubioOnset.setOnsetMinioi(millis/1000.0f);
    }
    if(boxThresh.hasButton(button))
    {
        float val = boxThresh.getText().getFloatValue();
        std::cout << "Setting Onset Threshold to " << val << " at next memory release/alloc" << std::endl;
        this->processor.aubioOnset.setOnsetThreshold(val);
    }
    if(boxSilenceThresh.hasButton(button))
    {
        float val = boxSilenceThresh.getText().getFloatValue();
        std::cout << "Setting Silence Threshold to " << val << " at next memory release/alloc" << std::endl;
        this->processor.aubioOnset.setSilenceThreshold(val);
    }
   #else
    if(boxDebounce.hasButton(button))
    {
        unsigned int millis = boxDebounce.getText().getIntValue();
        std::cout << "Setting debounce time to " << millis << " milliseconds" << std::endl;

        processor.bark.setDebounce(millis);
    }
    else if(boxThresh.hasButton(button))
    {
        float lo = boxThresh.getText1().getFloatValue();
        float hi = boxThresh.getText2().getFloatValue();
        std::cout << "Setting thresholds to " << lo << ", " << hi << std::endl;

        processor.bark.setThresh(lo,hi);
    }
   #endif
    else if(cluster.hasButton(button))
    {
        std::cout << "cluster pressed" << std::endl;
        unsigned int numClusters = cluster.getText().getIntValue();
        processor.knn.autoCluster(numClusters);
    }
    else if(&uncluster == button)
    {
        std::cout << "uncluster pressed" << std::endl;
        processor.knn.uncluster();
    }
    else if(&clearAll == button)
    {
        std::cout << "clearAll pressed" << std::endl;
        processor.knn.clearAll();
    }
    else if(write.hasButton(button))
    {
        std::cout << "write pressed" << std::endl;
        std::string path = write.getText().toStdString();
        std::cout << "path " << path << std::endl;
        processor.knn.writeData(path);
    }
    else if(read.hasButton(button))
    {
        std::cout << "read pressed" << std::endl;
        std::string path = read.getText().toStdString();
        std::cout << "path " << path << std::endl;
        processor.knn.readData(path);
    }
    else if(writeText.hasButton(button))
    {
        std::cout << "write Text pressed" << std::endl;
        std::string path = writeText.getText().toStdString();
        std::cout << "path " << path << std::endl;
        processor.knn.writeTextData(path);
    }
    else if(readText.hasButton(button))
    {
        std::cout << "read Text pressed" << std::endl;
        std::string path = readText.getText().toStdString();
        std::cout << "path " << path << std::endl;
        processor.knn.readTextData(path);
    }
    else if(featureNumber.hasButton(button))
    {
        processor.featuresUsed = featureNumber.getText().getIntValue();
    }
    else if(manualCluster.hasButton(button))
    {
        /******/
        /* How it works*/
        /*
         * When a string like "0,20,40,60,80" is read, it means that
         * the number of clusters is 4 (size -1).
         * Cluster 0 elements go from record 0 to record 19 (next number -1)
         * Cluster 1 elements go from record 20 to record 39 (next number -1)
         * Cluster 2 elements go from record 40 to record 59 (next number -1)
         * Cluster 3 elements go from record 60 to record 79 (next number -1)
        */
        /******/

        std::string indexes = manualCluster.getText().toStdString();
        std::cout << "read the string '" << indexes << "'\n";

        std::vector<std::string> indexesVec = splitString(indexes,',');
        std::cout << "done the split" << "\n";

        uint32_t numClusters = indexesVec.size() -1;

        uint32_t clusterIdx = 0;
        uint32_t lowIdx = std::stoi(indexesVec[0]);

        for(size_t i = 1; i < indexesVec.size(); ++i)
        {
            std::string s = indexesVec[i];
            std::cout << "> '" << s << "'\n";
            uint32_t highIdx = std::stoi(s);


            std::cout << "> manualCluster(" << numClusters << "," << clusterIdx << "," << lowIdx << "," << highIdx-1 << ")\n";
            processor.knn.manualCluster(numClusters,clusterIdx,lowIdx,highIdx-1);

            clusterIdx++;
            lowIdx = highIdx;
        }
    }
}

void DemoEditor::updateDataLabel(){
    std::string text = "";
   #ifdef USE_AUBIO_ONSET
    text += "Using AUBIO ONSET DETECTOR";
    text+=("\nWindow Size: " + std::to_string(processor.aubioOnset.getWindowSize()));
    text+=("\nHopSize: " + std::to_string(processor.aubioOnset.getHopSize()));
    text+=("\nSilence Threshold: " + std::to_string(processor.aubioOnset.getSilenceThreshold()));
    text+=("\nOnset Method: " + processor.aubioOnset.getStringOnsetMethod());
    text+=("\nOnset Threshold: " + std::to_string(processor.aubioOnset.getOnsetThreshold()));
    text+=("\nOnset Min Inter-Onset Interval: " + std::to_string(processor.aubioOnset.getOnsetMinInterOnsetInterval()));
   #else
    std::string sampleRate_s = std::to_string((int)this->mSampleRate);
    std::string blockSize_s = std::to_string((int)this->mBlockSize);
    std::string windowSize_s = std::to_string((int)this->processor.bark.getWindowSize());

    text += "Using BARK ONSET DETECTOR\n";
    text += "Sample rate:  "+sampleRate_s+"\nBlock size:  "+blockSize_s+"\nWindow size:  " +windowSize_s;
    text += "\nDebug mode: ";
    text += processor.bark.getDebugMode()?"True":"False";
    text += "\nSpew mode: ";
    text += processor.bark.getSpewMode()?"True":"False";
    text += "\nUse Weights: ";
    text += processor.bark.getUseWeights()?"True":"False";

    text += "\nDebounce time: ";
    text += std::to_string(processor.bark.getDebounceTime());
    text += " millis";

    text += "\nThresholds: ";
    auto threshs = processor.bark.getThresh();
    text += std::to_string(threshs.first);
    text += ", ";
    text += std::to_string(threshs.second);
   #endif


    this->dataLabel.setText(text,NotificationType::dontSendNotification);
}

void DemoEditor::updateBFCCDataLabel(){
    std::string text = processor.bfcc.getInfoString();
    text += "\n\nFeatures used: ";
    text += std::to_string(processor.featuresUsed);
    this->bfccInfo.setText(text,NotificationType::dontSendNotification);
}

void DemoEditor::comboBoxChanged(ComboBox* cb)
{
    if(cb == &classifierState)
    {
        switch (cb->getSelectedId()) {
            case 1:
                processor.classifierState = DemoProcessor::CState::train;
                break;
            case 2:
                processor.classifierState = DemoProcessor::CState::classify;
                break;
            case 3:
                processor.classifierState = DemoProcessor::CState::idle;
                break;
        }
    }
}

void DemoEditor::updateKnnLabel(unsigned int match, float distance)
{
    this->dispTimbre.setText("Timbre: " + std::to_string(match), NotificationType::dontSendNotification);
    this->dispDist.setText("Distance: " + std::to_string(distance), NotificationType::dontSendNotification);
}
