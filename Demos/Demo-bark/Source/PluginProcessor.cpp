/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - bark Module

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 25th March 2020

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <iostream>


//==============================================================================
DemoProcessor::DemoProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
    /** SET SOME DEFAULT PARAMETERS OF THE BARK MODULE **/
    bark.setDebounce(200);
    bark.setMask(4, 0.75);
    bark.setFilterRange(0, 49);
    bark.setThresh(-1, 30);

    /** ADD THE LISTENER **/
    bark.addListener(this);
}

DemoProcessor::~DemoProcessor(){    /*DESTRUCTOR*/        }

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    /** PREPARE THE BARK MODULE **/
    bark.prepare(sampleRate, (uint32)samplesPerBlock);
}

void DemoProcessor::releaseResources()
{
    /** RESET THE MODULE **/
    bark.reset();
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    /** STORE BUFFER **/
    try
    {
        /** STORE THE BUFFER FOR COMPUTATION and retrieving growthdata**/
        tid::Bark<float>::GrowthData growthData = bark.store(buffer,(short int)0);

        /** Retrieve the total growth and the growth data list **/
        std::vector<float>* growth = growthData.getGrowth();
        float* totalGrowth = growthData.getTotalGrowth();

        /** Check whether the data is valid: depending on analysis and
            whether spew mode is ON, the growthData will be provided
            at different calls
        **/
        if(growth != nullptr && totalGrowth != nullptr)
        {
            /** This is not a good example of how to log the list
                The standard stream output shouldn't be used from
                the audio thread.
            **/
#ifdef PRINT_GROWTH
            std::cout << *totalGrowth << " : ";

            std::string growth_s = "";
            for(int i=0; i<growth->size(); ++i)
            {
                growth_s += std::to_string(growth->at(i));
                if(i != growth->size()-1)
                    growth_s += ", ";
            }

            std::cout << growth_s << std::endl;
#endif // PRINT_GROWTH
        }

    }
    catch(std::exception& e)
    {
        std::cout << "An exception has occurred while buffering:" << std::endl;
        std::cout << e.what() << '\n';
    }
    catch(...)
    {
        std::cout << "An UNKNOWN exception has occurred while buffering" << std::endl;
    }

    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
}

/**
 * Onset Detected Callback
 * Remember that this is called from the audio thread so it must not update the GUI directly
**/
void DemoProcessor::onsetDetected (tid::Bark<float> * bark)
{
    if(bark == &this->bark)
        this->onsetMonitorState.exchange(true);
}
/*    JUCE stuff ahead, not relevant to the demo    */







//==============================================================================
const String DemoProcessor::getName() const{return JucePlugin_Name;}
bool DemoProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DemoProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}
bool DemoProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}
double DemoProcessor::getTailLengthSeconds() const{return 0.0;}
int DemoProcessor::getNumPrograms(){return 1;}
int DemoProcessor::getCurrentProgram(){return 0;}
void DemoProcessor::setCurrentProgram (int index){}
const String DemoProcessor::getProgramName (int index){return {};}
void DemoProcessor::changeProgramName (int index, const String& newName){}

//==============================================================================

#ifndef JucePlugin_PreferredChannelConfigurations
bool DemoProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    if (layouts.getMainOutputChannelSet() != AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif


//==============================================================================
bool DemoProcessor::hasEditor() const {return true;}
AudioProcessorEditor* DemoProcessor::createEditor(){return new DemoEditor (*this);}
void DemoProcessor::getStateInformation (MemoryBlock& destData){}
void DemoProcessor::setStateInformation (const void* data, int sizeInBytes){}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemoProcessor();
}
