#include "PluginEditor.h"
#include "PluginProcessor.h"

//==============================================================================
M1MonitorAudioProcessorEditor::M1MonitorAudioProcessorEditor(M1MonitorAudioProcessor& p)
    : AudioProcessorEditor(&p)
{
    setSize(504, 267);
    // size when advanced settings open = 504, 469

    processor = &p;

    // Whenever the processor wants to post an alert, it calls postAlertToUI
    // which we forward to our pannerUIBaseComponent:
    processor->postAlertToUI = [this](const Mach1::AlertData& alert)
    {
        if (monitorUIBaseComponent)
            monitorUIBaseComponent->postAlert(alert);
    };

    monitorUIBaseComponent = new MonitorUIBaseComponent(processor);
    monitorUIBaseComponent->setSize(getWidth(), getHeight());

    addAndMakeVisible(monitorUIBaseComponent);
    monitorUIBaseComponent->editor = this;

    // Add this to flush stored alerts:
    for (auto& alert : processor->pendingAlerts) {
        monitorUIBaseComponent->postAlert(alert);
    }
    processor->pendingAlerts.clear();
}

M1MonitorAudioProcessorEditor::~M1MonitorAudioProcessorEditor()
{
    monitorUIBaseComponent->shutdownOpenGL();
    removeAllChildren();
    delete monitorUIBaseComponent;
}

//==============================================================================

void M1MonitorAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(40, 40, 40));
    g.setColour(juce::Colour(40, 40, 40));
}

void M1MonitorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
