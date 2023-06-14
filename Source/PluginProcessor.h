/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "CircularBuffer.h"
#define delaySize 4
#define bandSize 11
#define M_PI    3.141592653589793238462643383279502884 

//==============================================================================
/**
*/
class nnAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    nnAudioProcessor();
    ~nnAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    pybind11::scoped_interpreter guard;

	std::unique_ptr<CircularBuffer<double>> CB1;
	std::unique_ptr<CircularBuffer<double>> CB2;
	std::unique_ptr<CircularBuffer<double>> CB3;
	std::unique_ptr<CircularBuffer<double>> CB4;

	std::unique_ptr<CircularBuffer<double>> FdnOutput_L;
	std::unique_ptr<CircularBuffer<double>> FdnOutput_R;

	double feedbackLoop1;
	double feedbackLoop2;
	double feedbackLoop3;
	double feedbackLoop4;

	std::vector<std::vector<juce::IIRFilter>> absorptionFilters;

	std::vector<juce::IIRFilter> initialFiltersL;
	std::vector<juce::IIRFilter> initialFiltersR;

	float delayLine1;
	float delayLine2;
	float delayLine3;
	float delayLine4;

	juce::AudioParameterFloat* Mix;
	juce::AudioParameterFloat* Volume;
	juce::AudioParameterFloat* Ratio;
	float nnAudioProcessor::processSignalThroughFilters(float xn, std::vector<juce::IIRFilter>& filters);
	juce::dsp::Convolution irloader;
	juce::dsp::ProcessSpec spec;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (nnAudioProcessor)
};
