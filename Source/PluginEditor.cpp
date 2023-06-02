/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include "JuceHeader.h"
#include <filesystem>

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
    addAndMakeVisible(btn_load_file);
    addAndMakeVisible(btn_convert_parameters);

    btn_load_file.onClick = [this] { openFileChooser(); };
    setSize(800, 600);

}

nnAudioProcessorEditor::~nnAudioProcessorEditor()
{
}

//==============================================================================
void nnAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll(getLookAndFeel().findColour(juce::ResizableWindow::backgroundColourId));
}

void nnAudioProcessorEditor::init_environment()
{
    // external.py is located at ~/NN_Func/Source
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path grandpaPath = currentPath.parent_path().parent_path();
    std::filesystem::path modulePath = grandpaPath / "Source";

    // edit environment path
    auto sys = pybind11::module_::import("sys");
    sys.attr("path").attr("insert")(0, modulePath.lexically_normal().string());
}

void nnAudioProcessorEditor::on_decode_room_impulse_response(std::string fp, float ch1, float ch2, float ch3, float ch4)
{    
    // import module
    auto external_module = pybind11::module::import("external");

    // execute python function
    pybind11::list pyList = external_module.attr("RIR2FDN")(fp, ch1, ch2, ch3, ch4).cast<pybind11::list>();
    auto data = convert_pylist_to_vector_3d(pyList);

    // assign data to private member
    absorption_coefs.assign(data.begin(), data.begin() + 4);
    transition_coefs = data[4];

    for (size_t channel = 0; channel < 4; channel++)
    {
        for (size_t band = 0; band < 11; band++)
        {
            table.update_entry(channel+1, band+1, absorption_coefs[channel][band]);
        }
    }

    for (size_t band = 0; band < 11; band++)
    {
        table.update_entry(5, band+1, transition_coefs[band]);
    }
}

void nnAudioProcessorEditor::disp_coefficient()
{
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

void nnAudioProcessorEditor::openFileChooser()
{
    const auto callback = [this](const juce::FileChooser& chooser)
    {
        //DBG("" << chooser.getResult().getFullPathName());
        loadData();
    };
    fileChooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, callback);
}

void nnAudioProcessorEditor::loadData()
{
    pybind11::scoped_interpreter guard{};
    init_environment();
    on_decode_room_impulse_response("C:\\Python37\\Lib\\DecayFitNet\\data\\exampleRIRs\\singleslope_00006_sh_rirs.wav", 1021, 2029, 3001, 4093);
    disp_coefficient();
}


void nnAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto buttonArea = getLocalBounds().removeFromTop(50).reduced(10);
    btn_load_file.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2).reduced(2));
    btn_convert_parameters.setBounds(buttonArea.reduced(2));
    table.setBounds(getLocalBounds().removeFromBottom(area.getHeight() - 50));
}
