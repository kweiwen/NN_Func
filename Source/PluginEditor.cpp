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
    //init_environment();
    addAndMakeVisible(&lbl_rir_path);
    addAndMakeVisible(&edt_rir_path);
    addAndMakeVisible(btn_load_rir);
    addAndMakeVisible(&lbl_py_path);
    addAndMakeVisible(&edt_py_path);
    addAndMakeVisible(btn_load_py);
    addAndMakeVisible(table);
    addAndMakeVisible(btn_convert_parameters);

    edt_py_path.setText("D:\\Project\\NN_Func\\Source");


    lbl_rir_path.setText("RIR(Room Impulse Response) Path: ", juce::dontSendNotification);
    lbl_py_path.setText("Neural Network Python Script Path: ", juce::dontSendNotification);
    edt_rir_path.setReadOnly(true);
    edt_py_path.setReadOnly(true);

	btn_convert_parameters.onClick = [this] {sync_impulse_response_n_coefficients(); };
    btn_load_rir.onClick = [this] { open_rir_chooser(); };
    btn_load_py.onClick = [this] { open_py_chooser(); };
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
    //std::filesystem::path currentPath = std::filesystem::current_path();
    //std::filesystem::path grandpaPath = currentPath.parent_path().parent_path();
    //std::filesystem::path modulePath = grandpaPath / "Source";

    // edit environment path
}

void nnAudioProcessorEditor::on_decode_room_impulse_response(std::string fp, float ch1, float ch2, float ch3, float ch4)
{    
    auto sys = pybind11::module_::import("sys");
    sys.attr("path").attr("insert")(0, "D:\\Project\\NN_Func\\Source");

    // import module
    auto external_module = pybind11::module_::import("external");

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

void nnAudioProcessorEditor::open_rir_chooser()
{
	const auto callback = [this](const juce::FileChooser& chooser)
	{
		if (chooser.getResult().getFileExtension() == ".wav" || chooser.getResult().getFileExtension() == ".mp3")
		{
			table.clean_entry();
			on_decode_room_impulse_response(chooser.getResult().getFullPathName().toStdString(), audioProcessor.delayLine1, audioProcessor.delayLine2, audioProcessor.delayLine3, audioProcessor.delayLine4);
            edt_rir_path.setText(chooser.getResult().getFullPathName());
			disp_coefficient();
			result = chooser.getResult();
		}	
    };
    fileChooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, callback);

	auto ColourId1 = juce::Colours::darkseagreen;
	btn_convert_parameters.setColour(0x1000100, ColourId1);
}

void nnAudioProcessorEditor::open_py_chooser()
{
    const auto callback = [this](const juce::FileChooser& chooser)
    {
        if (chooser.getResult().getFileExtension() == ".py")
        {
        }
    };
    fileChooser.launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles, callback);
}

void nnAudioProcessorEditor::sync_impulse_response_n_coefficients()
{
	auto ColourId1 = juce::Colours::yellowgreen;
	btn_convert_parameters.setColour(0x1000100, ColourId1);

	audioProcessor.convolution.reset();
	audioProcessor.convolution.loadImpulseResponse(result, juce::dsp::Convolution::Stereo::yes, juce::dsp::Convolution::Trim::no, 0);

	for (size_t i = 0; i < absorption_coefs.size(); ++i)
	{
		for (size_t j = 0; j < absorption_coefs[i].size(); ++j)
		{
            auto b0 = absorption_coefs[i][j][0];
            auto b1 = absorption_coefs[i][j][1];
            auto b2 = absorption_coefs[i][j][2];

            auto a0 = absorption_coefs[i][j][3];
            auto a1 = absorption_coefs[i][j][4];
            auto a2 = absorption_coefs[i][j][5];

            juce::IIRCoefficients coeffs(b0, b1, b2, a0, a1, a2);
            audioProcessor.absorptionFilters[i][j].setCoefficients(coeffs);
		}
	}

	for (size_t j = 0; j < transition_coefs.size(); ++j)
	{
        auto b0 = transition_coefs[j][0];
        auto b1 = transition_coefs[j][1];
        auto b2 = transition_coefs[j][2];
        
        auto a0 = transition_coefs[j][3];
        auto a1 = transition_coefs[j][4];
        auto a2 = transition_coefs[j][5];

        juce::IIRCoefficients coeffs(b0, b1, b2, a0, a1, a2);
        audioProcessor.initialFiltersL[j].setCoefficients(coeffs);
        audioProcessor.initialFiltersR[j].setCoefficients(coeffs);
	}
}

void nnAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();
    auto topArea = area.removeFromTop(118);

    auto buttonArea = topArea.removeFromTop(42).reduced(5);
    //btn_load_file.setBounds(buttonArea.removeFromLeft(buttonArea.getWidth() / 2).reduced(2));
    btn_convert_parameters.setBounds(buttonArea.reduced(2));

    auto rirPathArea = topArea.removeFromTop(38).reduced(5);
    lbl_rir_path.setBounds(rirPathArea.removeFromLeft(240));
    edt_rir_path.setBounds(rirPathArea.removeFromLeft(rirPathArea.getWidth() - 30));
    btn_load_rir.setBounds(rirPathArea);

    auto pyPathArea = topArea.removeFromTop(38).reduced(5);
    lbl_py_path.setBounds(pyPathArea.removeFromLeft(240));
    edt_py_path.setBounds(pyPathArea.removeFromLeft(pyPathArea.getWidth() - 30));
    btn_load_py.setBounds(pyPathArea);

    table.setBounds(area);
}

