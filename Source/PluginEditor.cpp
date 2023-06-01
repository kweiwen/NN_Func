/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "JuceHeader.h"

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
    addAndMakeVisible(table);
    setSize(1200, 600);

    pybind11::scoped_interpreter guard{};
        
    on_decode_room_impulse_response("");

    for (size_t i = 0; i < absorption_coefs.size(); ++i) 
    {
        for (size_t j = 0; j < absorption_coefs[i].size(); ++j) 
        {
            for (size_t k = 0; k < absorption_coefs[i][j].size(); ++k) 
            {
                DBG("Element at index (" << i << ", " << j << ", " << k << ") = " << absorption_coefs[i][j][k]);
            }
        }
    }

    for (size_t j = 0; j < transition_coefs.size(); ++j)
    {
        for (size_t k = 0; k < transition_coefs[j].size(); ++k)
        {
            DBG("Element at index (" << ", " << j << ", " << k << ") = " << transition_coefs[j][k]);
        }
    }

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
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    //g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //g.setColour (juce::Colours::white);
    //g.setFont (15.0f);

    //g.drawFittedText ("Hello World!",                   getLocalBounds(), juce::Justification::centred, 1);
    //g.drawFittedText (juce::String(ext_decode_data),  getLocalBounds(), juce::Justification::centredBottom, 1);
}

void nnAudioProcessorEditor::on_decode_room_impulse_response(juce::String fp)
{
    // setup environment path
    auto sys = pybind11::module_::import("sys");
    sys.attr("path").attr("insert")(0, "D:\\Project\\NN_Func\\Source");
    // import module
    auto module = pybind11::module_::import("external");

    pybind11::list pyList = module.attr("demo")().cast<pybind11::list>();
    auto data = convert_pylist_to_vector_3d(pyList);
    // assign data to private member
    absorption_coefs.assign(data.begin(), data.begin() + 4);
    transition_coefs = data[4];
}


std::vector<std::vector<std::vector<float>>> nnAudioProcessorEditor::convert_pylist_to_vector_3d(pybind11::list pylist)
{
    std::vector<std::vector<std::vector<float>>> output_data(pylist.size());

    for (size_t i = 0; i < pylist.size(); i++) 
    {
        output_data[i] = std::vector<std::vector<float>>(pylist[i].cast<pybind11::list>().size());
        for (size_t j = 0; j < pylist[i].cast<pybind11::list>().size(); j++) 
        {
            output_data[i][j] = std::vector<float>(pylist[i].cast<pybind11::list>()[j].cast<pybind11::list>().size());
            for (size_t k = 0; k < pylist[i].cast<pybind11::list>()[j].cast<pybind11::list>().size(); k++) 
            {
                output_data[i][j][k] = pylist[i].cast<pybind11::list>()[j].cast<pybind11::list>()[k].cast<float>();
            }
        }
    }
    return output_data;
}

void nnAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
    table.setBounds(getLocalBounds());
}
