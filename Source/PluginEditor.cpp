/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
M1MonitorAudioProcessorEditor::M1MonitorAudioProcessorEditor (M1MonitorAudioProcessor& p)
    : AudioProcessorEditor (&p)
{
    setSize(504, 180);
    //juce::OpenGLAppComponent::setSize(504, 266);
    // size when advanced settings open = 504, 469
    
    processor = &p;
    
    monitorUIBaseComponent = new MonitorUIBaseComponent(processor);
    monitorUIBaseComponent->setSize(getWidth(), getHeight());

    addAndMakeVisible(monitorUIBaseComponent);
    
    monitorUIBaseComponent->windowResize = [&](int w, int h) {
        MessageManager::callAsync([&]() {
            setSize(w, h);
            monitorUIBaseComponent->setSize(getWidth(), getHeight());
        });
    };
}

M1MonitorAudioProcessorEditor::~M1MonitorAudioProcessorEditor()
{
    monitorUIBaseComponent->shutdownOpenGL();
    removeAllChildren();
    delete monitorUIBaseComponent;
}

//==============================================================================

void M1MonitorAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (40, 40, 40));
    g.setColour (juce::Colour (40, 40, 40));
    
    monitorUIBaseComponent->windowResize = [&](int w, int h) {
        MessageManager::callAsync([&]() {
            setBounds(getBounds().getX(), getBounds().getY(), w, h);
        });
    };
}

void M1MonitorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
