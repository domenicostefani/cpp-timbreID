/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

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
}

DemoProcessor::~DemoProcessor()
{
}

//==============================================================================
const String DemoProcessor::getName() const
{
    return JucePlugin_Name;
}

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

double DemoProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int DemoProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int DemoProcessor::getCurrentProgram()
{
    return 0;
}

void DemoProcessor::setCurrentProgram (int index)
{
}

const String DemoProcessor::getProgramName (int index)
{
    return {};
}

void DemoProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void DemoProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    zeroCrossing.prepare(sampleRate, (uint32)samplesPerBlock);
    mono.setSize (1, samplesPerBlock);
}

void DemoProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
    zeroCrossing.reset();
}

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

void DemoProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());


    mono.copyFrom(0,0,buffer,0,0,buffer.getNumSamples());

    dsp::AudioBlock<float> block (mono);
    zeroCrossing.store(dsp::ProcessContextReplacing<float>(block));
}

//==============================================================================
bool DemoProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

AudioProcessorEditor* DemoProcessor::createEditor()
{
    return new DemoEditor (*this);
}

//==============================================================================
void DemoProcessor::getStateInformation (MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void DemoProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new DemoProcessor();
}

uint32 DemoProcessor::computeZeroCrossing(){
    return zeroCrossing.countCrossings();
}
