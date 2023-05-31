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
    std::vector<std::vector<std::vector<float>>> convert_pylist_to_vector_3d(pybind11::list pylist);
    std::vector<std::vector<std::vector<float>>> absorption_coefs;
    std::vector<std::vector<float>> transition_coefs;
    
    int int_decode_data;
    float ext_decode_data;
    TableTutorialComponent table;


    //juce::ListBox* listBox;
    //std::vector<std::vector<float>> myData;
    //MyListBoxModel myModel;
    //juce::TableListBox table{ {}, nullptr };
    //MyTableModel tableModel;
    //std::vector<std::vector<float>> data = { {1.0f, 2.0f, 3.0f, 4.0f, 5.0f},
    //                                         {1.1f, 2.1f, 3.1f, 4.1f, 5.1f},
    //                                         {1.2f, 2.2f, 3.2f, 4.2f, 5.2f} };


    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (nnAudioProcessorEditor)
};
