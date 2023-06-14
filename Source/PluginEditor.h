/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include <vector>
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

    void init_environment();
    void on_decode_room_impulse_response(std::string fp, float ch1, float ch2, float ch3, float ch4);
    void disp_coefficient();
    std::vector<std::vector<std::vector<float>>> convert_pylist_to_vector_3d(pybind11::list pylist);
    std::vector<std::vector<std::vector<float>>> absorption_coefs;
    std::vector<std::vector<float>> transition_coefs;
    void open_rir_chooser();
    void open_py_chooser();
    void sync_impulse_response_n_coefficients();

    //pybind11::object external_module;
	
    int int_decode_data;
    float ext_decode_data;
    CoefficientTableComponent table;
    juce::Label lbl_rir_path;
    juce::TextEditor edt_rir_path;

    juce::Label lbl_py_path;
    juce::TextEditor edt_py_path;
    
    juce::TextButton btn_load_rir{ "..." };
    juce::TextButton btn_load_py{ "..." };

    juce::TextButton btn_convert_parameters{ "Convert Parameters" };
	juce::File result;
    juce::FileChooser fileChooser{ "Browse for Room Imoulse Response Data", juce::File::getSpecialLocation(juce::File::invokedExecutableFile) };
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (nnAudioProcessorEditor)
};
