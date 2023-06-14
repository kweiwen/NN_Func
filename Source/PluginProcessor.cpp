/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
nnAudioProcessor::nnAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{

	addParameter(level1 = new juce::AudioParameterFloat("0x01", "dry", 0.00f, 1.00f, 1.00f));
	addParameter(level2 = new juce::AudioParameterFloat("0x02", "convolution", 0.00f, 1.00f, 1.00f));
	addParameter(level3 = new juce::AudioParameterFloat("0x03", "feedback delay network", 0.00f, 1.00f, 1.00f));
}

nnAudioProcessor::~nnAudioProcessor()
{
}

//==============================================================================
const juce::String nnAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool nnAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool nnAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool nnAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double nnAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int nnAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int nnAudioProcessor::getCurrentProgram()
{
    return 0;
}

void nnAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String nnAudioProcessor::getProgramName (int index)
{
    return {};
}

void nnAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void nnAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
	// init feedback delay network
	CB1.reset(new CircularBuffer<double>);
	CB2.reset(new CircularBuffer<double>);
	CB3.reset(new CircularBuffer<double>);
	CB4.reset(new CircularBuffer<double>);
	
	CB1->createCircularBuffer(4096);
	CB1->flushBuffer();
	
	CB2->createCircularBuffer(4096);
	CB2->flushBuffer();
	
	CB3->createCircularBuffer(4096);
	CB3->flushBuffer();
	
	CB4->createCircularBuffer(4096);
	CB4->flushBuffer();
	
	bufferL.resize(samplesPerBlock);
	bufferR.resize(samplesPerBlock);
	dryL.resize(samplesPerBlock);
	dryR.resize(samplesPerBlock);
	
	feedbackLoop1 = 0.0f;
	feedbackLoop2 = 0.0f;
	feedbackLoop3 = 0.0f;
	feedbackLoop4 = 0.0f;

	delayLine1 = 2003;
	delayLine2 = 2011;
	delayLine3 = 4049;
	delayLine4 = 4051;

	absorptionFilters.resize(delaySize);
	for (auto& filter : absorptionFilters)
	{
		filter.resize(bandSize);
	}

	initialFiltersL.resize(bandSize);
	initialFiltersR.resize(bandSize);

	// init convolution
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = getTotalNumOutputChannels();
	convolution.reset();
	convolution.prepare(spec);
}

void nnAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool nnAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
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

void nnAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;

	auto blockSize = buffer.getNumSamples();

	auto* inputL  = buffer.getReadPointer(0);
	auto* inputR  = buffer.getReadPointer(1);
	auto* outputL = buffer.getWritePointer(0);
	auto* outputR = buffer.getWritePointer(1);
	
	// store dry signal
	for (int i = 0; i < blockSize; i++)
	{
		dryL[i] = inputL[i];
		dryR[i] = inputR[i];
	}

	// feedback delay network Process
	for (int i = 0; i < blockSize; i++)
	{
		feedbackLoop1 = CB1->readBuffer(delayLine1, false);
		feedbackLoop2 = CB2->readBuffer(delayLine2, false);
		feedbackLoop3 = CB3->readBuffer(delayLine3, false);
		feedbackLoop4 = CB4->readBuffer(delayLine4, false);

		auto A = processSignalThroughFilters(feedbackLoop1, absorptionFilters[0]);
		auto B = processSignalThroughFilters(feedbackLoop2, absorptionFilters[1]);
		auto C = processSignalThroughFilters(feedbackLoop3, absorptionFilters[2]);
		auto D = processSignalThroughFilters(feedbackLoop4, absorptionFilters[3]);

		auto output_1 = 0.5f * (A + B + C + D);
		auto output_2 = 0.5f * (A - B + C - D);
		auto output_3 = 0.5f * (A + B - C - D);
		auto output_4 = 0.5f * (A - B - C + D);

		CB1->writeBuffer(dryL[i] + output_1);
		CB2->writeBuffer(dryR[i] + output_2);
		CB3->writeBuffer(output_3);
		CB4->writeBuffer(output_4);

		bufferL[i] = processSignalThroughFilters(A + D, initialFiltersL);
		bufferR[i] = processSignalThroughFilters(B + C, initialFiltersL);
	}

	// convolution process
	juce::dsp::AudioBlock<float> block{ buffer };
	convolution.process(juce::dsp::ProcessContextReplacing<float>(block));
	auto* convL = block.getChannelPointer(0);
	auto* convR = block.getChannelPointer(1);

	// output
	for (int i = 0; i < block.getNumSamples(); i++)
	{
		outputL[i] = dryL[i] * level1->get() + convL[i] * level2->get() + bufferL[i] * 3.0f * level3->get();
		outputR[i] = dryR[i] * level1->get() + convR[i] * level2->get() + bufferR[i] * 3.0f * level3->get();
	}	
}

//==============================================================================
bool nnAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* nnAudioProcessor::createEditor()
{
    return new nnAudioProcessorEditor (*this);
}

//==============================================================================
void nnAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void nnAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new nnAudioProcessor();
}

float nnAudioProcessor::processSignalThroughFilters(float xn, std::vector<juce::IIRFilter>& filters)
{
	for (auto& filter : filters) 
	{
		xn = filter.processSingleSampleRaw(xn);
	}
	return xn;
}
