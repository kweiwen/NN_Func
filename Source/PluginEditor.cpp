/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "JuceHeader.h"
#include <pybind11/embed.h>
#include <pybind11/pybind11.h>
//namespace py = pybind11;

PYBIND11_EMBEDDED_MODULE(local_calc, m) {
    // `m` is a `py::module_` which is used to bind functions and classes
    m.def("add", [](int i, int j) {
        return i + j;
        });
}

//==============================================================================
nnAudioProcessorEditor::nnAudioProcessorEditor (nnAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (400, 300);

    pybind11::scoped_interpreter guard{};
        
    // init local function
    // import module
    //auto local_calc = pybind11::module_::import("local_calc");
    //int_decode_data = local_calc.attr("add")(5, 4).cast<int>();

    // init external function
    // add path 
    auto sys = pybind11::module_::import("sys");
    sys.attr("path").attr("insert")(0, "D:\\Project\\NN_Func\\Source");
    // import module
    auto module = pybind11::module_::import("calc");
    ext_decode_data = module.attr("add")(66, 6).cast<int>();

    //DBG(int_decode_data);
    DBG(ext_decode_data);
    DBG("TEST");
}

nnAudioProcessorEditor::~nnAudioProcessorEditor()
{
}

//==============================================================================
void nnAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);

    g.drawFittedText ("Hello World!",                   getLocalBounds(), juce::Justification::centred, 1);
    g.drawFittedText (std::to_string(ext_decode_data),  getLocalBounds(), juce::Justification::centredBottom, 1);
}

void nnAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
