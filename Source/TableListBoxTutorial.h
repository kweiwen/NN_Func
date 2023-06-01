#pragma once
#include <JuceHeader.h>
#include <iostream>
#include <string>


class DataEntry
{
public:
    DataEntry(const std::vector<float>& values) : data(values)
    {
    }

    std::vector<float> data;
};

class TableTutorialComponent    : public juce::Component,
                                  public juce::TableListBoxModel
{
public:
    TableTutorialComponent()
    {
        coefficients = { {0,1,2,3,4,5},{0,1,2,3,4,5} };

        for (const auto& vec : coefficients)
            entries.push_back(DataEntry(vec));

        addAndMakeVisible(table);
        table.setColour (juce::ListBox::outlineColourId, juce::Colours::grey); 
        table.setOutlineThickness (1);
        table.getHeader().addColumn("channel",  1, 100, 50, 400, juce::TableHeaderComponent::notSortable && juce::TableHeaderComponent::draggable);
        table.getHeader().addColumn("band",     2, 50, 50, 400, juce::TableHeaderComponent::notSortable && juce::TableHeaderComponent::draggable);
        table.getHeader().addColumn("a0",       3, 50, 50, 400, juce::TableHeaderComponent::notSortable && juce::TableHeaderComponent::draggable);
        table.getHeader().addColumn("a1",       4, 50, 50, 400, juce::TableHeaderComponent::notSortable && juce::TableHeaderComponent::draggable);
        table.getHeader().addColumn("a2",       5, 50, 50, 400, juce::TableHeaderComponent::notSortable && juce::TableHeaderComponent::draggable);
        table.getHeader().addColumn("b0",       6, 50, 50, 400, juce::TableHeaderComponent::notSortable && juce::TableHeaderComponent::draggable);
        table.getHeader().addColumn("b1",       7, 50, 50, 400, juce::TableHeaderComponent::notSortable && juce::TableHeaderComponent::draggable);
        table.getHeader().addColumn("b2",       8, 50, 50, 400, juce::TableHeaderComponent::notSortable && juce::TableHeaderComponent::draggable);

        resized();
    }

    int getNumRows() override
    {
        return entries.size();
    }

    void paintRowBackground (juce::Graphics& g, int rowNumber, int /*width*/, int /*height*/, bool rowIsSelected) override
    {
        auto alternateColour = getLookAndFeel().findColour (juce::ListBox::backgroundColourId)
                                               .interpolatedWith (getLookAndFeel().findColour (juce::ListBox::textColourId), 0.03f);
        if (rowIsSelected)
            g.fillAll (juce::Colours::lightblue);
        else if (rowNumber % 2)
            g.fillAll (alternateColour);
    }

    void paintCell (juce::Graphics& g, int rowNumber, int columnId,
                    int width, int height, bool rowIsSelected) override
    {
        g.setColour(rowIsSelected ? juce::Colours::darkblue : getLookAndFeel().findColour(juce::ListBox::textColourId));
        g.setFont(font);

        if (columnId <= entries[rowNumber].data.size())
        {
            const juce::String text = juce::String(entries[rowNumber].data[columnId - 1]);
            g.drawText(text, 2, 0, width - 4, height, juce::Justification::centredLeft, true);
        }

        g.setColour(getLookAndFeel().findColour(juce::ListBox::backgroundColourId));
        g.fillRect(width - 1, 0, 1, height);
    }

    void resized() override
    {
        table.setBoundsInset (juce::BorderSize<int> (8));
    }

private:
    juce::TableListBox table  { {}, this };
    juce::Font font           { 14.0f };

    std::vector<std::vector<float>> coefficients;
    std::vector<DataEntry> entries;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TableTutorialComponent)
};