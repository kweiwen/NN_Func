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
#define NumberDelays 4
#define NumberBands 11
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

	std::unique_ptr<CircularBuffer<double>> CB_1;
	std::unique_ptr<CircularBuffer<double>> CB_2;
	std::unique_ptr<CircularBuffer<double>> CB_3;
	std::unique_ptr<CircularBuffer<double>> CB_4;

	std::unique_ptr<CircularBuffer<double>> FdnOutput_L;
	std::unique_ptr<CircularBuffer<double>> FdnOutput_R;

	double feedbackLoop_1;
	double feedbackLoop_2;
	double feedbackLoop_3;
	double feedbackLoop_4;

	typedef struct Filter_Coeff
	{
		double b0;
		double b1;
		double b2;
		double a1;
		double a2;
	}Filter_Coeff;

	typedef struct Filter_XY
	{
		float xn1;
		float xn2;
		float yn1;
		float yn2;
	}Filter_XY;

	typedef struct GEQ_Filter
	{
		Filter_Coeff filterCoeff[NumberDelays][NumberBands];
		Filter_XY  filterStatus[NumberDelays][NumberBands];

	}GEQ_Filter;

	GEQ_Filter AbsorptionFilter;
	GEQ_Filter InitialLevelFilter;

	float DelayLine1 = 2000;
	float DelayLine2 = 1433;
	float DelayLine3 = 556;
	float DelayLine4 = 2500;

	void MyFilter_Init(GEQ_Filter *pFilter);
	double MyFilter_Process(GEQ_Filter *pFilter, double pSampleIn, int DelayIndex);
	
	juce::AudioParameterFloat* Mix;
	juce::AudioParameterFloat* Volume;
	juce::AudioParameterFloat* Ratio;

	juce::dsp::Convolution irloader;
	juce::dsp::ProcessSpec spec;
private:
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (nnAudioProcessor)
};
