/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "juce_timbreID.h"

#define MONO_CHANNEL 1

//==============================================================================
DemoaubioonsetAudioProcessor::DemoaubioonsetAudioProcessor()
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

DemoaubioonsetAudioProcessor::~DemoaubioonsetAudioProcessor()
{
}

void DemoaubioonsetAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    aubioOnset.addListener(this);
    aubioOnset.prepare(sampleRate,samplesPerBlock); // Important step
}

void DemoaubioonsetAudioProcessor::releaseResources()
{
    aubioOnset.reset(); // Important step
}

void DemoaubioonsetAudioProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer& midiMessages)
{
    aubioOnset.store(buffer,MONO_CHANNEL);
}

void DemoaubioonsetAudioProcessor::onsetDetected (tid::aubio::Onset<float> *)
{
    std::cout << ("Onset Detected " + std::to_string(juce::Time::getApproximateMillisecondCounter())) << "\n";

}










#ifndef JucePlugin_PreferredChannelConfigurations
bool DemoaubioonsetAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

bool DemoaubioonsetAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool DemoaubioonsetAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool DemoaubioonsetAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

const String DemoaubioonsetAudioProcessor::getName() const { return JucePlugin_Name; }
double DemoaubioonsetAudioProcessor::getTailLengthSeconds() const { return 0.0; }
int DemoaubioonsetAudioProcessor::getNumPrograms() { return 1; }
int DemoaubioonsetAudioProcessor::getCurrentProgram() { return 0; }
void DemoaubioonsetAudioProcessor::setCurrentProgram (int index) { }
const String DemoaubioonsetAudioProcessor::getProgramName (int index) { return {}; }
void DemoaubioonsetAudioProcessor::changeProgramName (int index, const String& newName){}
bool DemoaubioonsetAudioProcessor::hasEditor() const { return true; }
AudioProcessorEditor* DemoaubioonsetAudioProcessor::createEditor() { return new DemoaubioonsetAudioProcessorEditor (*this); }
void DemoaubioonsetAudioProcessor::getStateInformation (MemoryBlock& destData) { }
void DemoaubioonsetAudioProcessor::setStateInformation (const void* data, int sizeInBytes) { }
AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DemoaubioonsetAudioProcessor(); }

//==============================================================================
