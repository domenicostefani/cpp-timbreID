/*
  ==============================================================================

  Plugin Processor

  DEMO PROJECT - TimbreID - bfcc Module (Bark Frequency Cepstral Coefficients)

  Author: Domenico Stefani (domenico.stefani96 AT gmail.com)
  Date: 23rd April 2020

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
}

DemoProcessor::~DemoProcessor(){    /*DESTRUCTOR*/        }

void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    /** PREPARE THE BARK MODULE **/
    bfcc.prepare(sampleRate, (uint32)samplesPerBlock);
}

void DemoProcessor::releaseResources()
{
    /** RESET THE MODULE **/
    bfcc.reset();
}

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;

    /** STORE THE BUFFER FOR ANALYSIS **/
    bfcc.store(buffer,(short int)0);

    // Look for the call to the compute() method in PluginEditor.cpp

    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = 0; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());
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
