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

	addParameter(Mix = new juce::AudioParameterFloat("0x01", "DirMix", 0.00f, 1.00f, 1.00f));
	addParameter(Volume = new juce::AudioParameterFloat("0x02", "FdnVolume", 1.00f, 30.00f, 3.10f));
	addParameter(Ratio = new juce::AudioParameterFloat("0x03", "Conv|Fdn", 0.00f, 1.00f, 1.00f));
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
	CB_1.reset(new CircularBuffer<double>);
	CB_2.reset(new CircularBuffer<double>);
	CB_3.reset(new CircularBuffer<double>);
	CB_4.reset(new CircularBuffer<double>);
	
	FdnOutput_L.reset(new CircularBuffer<double>);
	FdnOutput_R.reset(new CircularBuffer<double>);
	
	CB_1->createCircularBuffer(4096);
	CB_1->flushBuffer();
	
	CB_2->createCircularBuffer(4096);
	CB_2->flushBuffer();
	
	CB_3->createCircularBuffer(4096);
	CB_3->flushBuffer();
	
	CB_4->createCircularBuffer(4096);
	CB_4->flushBuffer();
	
	FdnOutput_L->createCircularBuffer(1024);
	FdnOutput_L->flushBuffer();
	FdnOutput_R->createCircularBuffer(1024);
	FdnOutput_R->flushBuffer();
	
	feedbackLoop_1 = 0.0f;
	feedbackLoop_2 = 0.0f;
	feedbackLoop_3 = 0.0f;
	feedbackLoop_4 = 0.0f;

	DelayLine1 = 2003;
	DelayLine2 = 2011;
	DelayLine3 = 4049;
	DelayLine4 = 4051;

	initialFiltersL.resize(bandSize);

	// init convolution
	spec.sampleRate = sampleRate;
	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = getTotalNumOutputChannels();
	irloader.reset();
	irloader.prepare(spec);
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
	juce::dsp::AudioBlock<float> block{ buffer };

	auto blockSize = buffer.getNumSamples();
	auto* input_L = buffer.getReadPointer(0);
	auto* input_R = buffer.getReadPointer(1);

	auto mix = Mix->get();
	auto vol = Volume->get();
	auto ratio = Ratio->get();
	// FDN Process
	for (int i = 0; i < blockSize; i++)
	{
		auto X_L = input_L[i];
		auto X_R = input_R[i];

		feedbackLoop_1 = CB_1->readBuffer(DelayLine1, false);
		feedbackLoop_2 = CB_2->readBuffer(DelayLine2, false);
		feedbackLoop_3 = CB_3->readBuffer(DelayLine3, false);
		feedbackLoop_4 = CB_4->readBuffer(DelayLine4, false);

		auto A = MyFilter_Process(&AbsorptionFilter, feedbackLoop_1, 0);
		auto B = MyFilter_Process(&AbsorptionFilter, feedbackLoop_2, 1);
		auto C = MyFilter_Process(&AbsorptionFilter, feedbackLoop_3, 2);
		auto D = MyFilter_Process(&AbsorptionFilter, feedbackLoop_4, 3);

		auto output_1 = 0.5f * (A + B + C + D);
		auto output_2 = 0.5f * (A - B + C - D);
		auto output_3 = 0.5f * (A + B - C - D);
		auto output_4 = 0.5f * (A - B - C + D);

		CB_1->writeBuffer(output_1);
		CB_2->writeBuffer(output_2);
		CB_3->writeBuffer(X_L + output_3);
		CB_4->writeBuffer(X_R + output_4);

		FdnOutput_L->writeBuffer(vol * processSignalThroughFilters(A + D, initialFiltersL));
		FdnOutput_R->writeBuffer(vol * processSignalThroughFilters(B + C, initialFiltersL));
	}
	irloader.process(juce::dsp::ProcessContextReplacing<float>(block));

	// Convolution
	auto* samples_L = block.getChannelPointer(0);
	auto* samples_R = block.getChannelPointer(1);
	for (int i = 0; i < block.getNumSamples(); i++)
	{
		samples_L[i] = input_L[i] * (1 - mix) + (1 - ratio)*samples_L[i] + ratio * FdnOutput_L->readBuffer(blockSize - i, false);
		samples_R[i] = input_R[i] * (1 - mix) + (1 - ratio)*samples_R[i] + ratio * FdnOutput_R->readBuffer(blockSize - i, false);
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
void nnAudioProcessor::MyFilter_Init(GEQ_Filter *pFilter)
{
	int i, j;
	memset(pFilter, 0, sizeof(GEQ_Filter));

	for (i = 0; i < delaySize; i++)
	{
		for (j = 0; j < bandSize; j++)
		{
			pFilter->filterCoeff[i][j].b0 = 1.0f;
		}
	}
}

double nnAudioProcessor::MyFilter_Process(GEQ_Filter *pFilter, double pSampleIn, int DelayIndex)
{
	double  b0, b1, b2, a1, a2;
	double accum;
	double xn, xn1, xn2, yn1, yn2;
	int j;

	xn = pSampleIn;

	for (j = 0; j < bandSize; j++)
	{
		b0 = pFilter->filterCoeff[DelayIndex][j].b0;
		b1 = pFilter->filterCoeff[DelayIndex][j].b1;
		b2 = pFilter->filterCoeff[DelayIndex][j].b2;
		a1 = pFilter->filterCoeff[DelayIndex][j].a1;
		a2 = pFilter->filterCoeff[DelayIndex][j].a2;


		xn1 = pFilter->filterStatus[DelayIndex][j].xn1;
		xn2 = pFilter->filterStatus[DelayIndex][j].xn2;
		yn1 = pFilter->filterStatus[DelayIndex][j].yn1;
		yn2 = pFilter->filterStatus[DelayIndex][j].yn2;


		accum = xn * b0;
		accum += xn1 * b1;
		accum += xn2 * b2;
		accum -= yn1 * a1;
		accum -= yn2 * a2;


		xn2 = xn1;
		xn1 = xn;
		yn2 = yn1;
		yn1 = accum;

		pFilter->filterStatus[DelayIndex][j].xn1 = xn1;
		pFilter->filterStatus[DelayIndex][j].xn2 = xn2;
		pFilter->filterStatus[DelayIndex][j].yn1 = yn1;
		pFilter->filterStatus[DelayIndex][j].yn2 = yn2;

		xn = accum;

	}
	return accum;
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
