/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <vector>
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "TableListBoxTutorial.h"

//==============================================================================
/**
*/
class nnAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    nnAudioProcessorEditor (nnAudioProcessor&);
    ~nnAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    nnAudioProcessor& audioProcessor;

    void on_decode_room_impulse_response(juce::String fp);
    void disp_coefficient();
    std::vector<std::vector<std::vector<float>>> convert_pylist_to_vector_3d(pybind11::list pylist);
    std::vector<std::vector<std::vector<float>>> absorption_coefs;
    std::vector<std::vector<float>> transition_coefs;
    
    int int_decode_data;
    float ext_decode_data;
    CoefficientTableComponent table;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (nnAudioProcessorEditor)
};
