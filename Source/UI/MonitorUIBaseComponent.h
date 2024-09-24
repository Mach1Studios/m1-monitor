/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "juce_murka/JuceMurkaBaseComponent.h"

#include "../Config.h"
#include "../M1Analytics.h"
#include "../PluginProcessor.h"
#include "../TypesForDataExchange.h"

#include "M1Checkbox.h"
#include "M1DropdownButton.h"
#include "M1DropdownMenu.h"
#include "M1Radial.h"
#include "M1Slider.h"
#include "m1_orientation_client/UI/M1Label.h"
#include "m1_orientation_client/UI/M1OrientationClientWindow.h"
#include "m1_orientation_client/UI/M1OrientationWindowToggleButton.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MonitorUIBaseComponent : public JuceMurkaBaseComponent, public juce::Timer
{
    M1MonitorAudioProcessor* processor = nullptr;
    MixerSettings* monitorState = nullptr;

public:
    //==============================================================================
    MonitorUIBaseComponent(M1MonitorAudioProcessor* processor);
    ~MonitorUIBaseComponent();

    juce::AudioProcessorEditor* editor;

    //==============================================================================
    void initialise() override;
    void draw() override;
    //==============================================================================
    void paint(juce::Graphics& g) override;
    void resized() override;

    bool shouldResizeComponent = false;
    MurkaPoint targetSize;

    void setShouldResizeTo(MurkaPoint size)
    {
        if ((getWidth() != size.x) || (getHeight() != size.y))
        {
            targetSize = size;
            shouldResizeComponent = true;
        }
    }

    // Timer callback for resizing
    void timerCallback() override;

    // Analytics for crash reporting
    void addJob(std::function<void()> job);
    void addJob(juce::ThreadPoolJob* job, bool deleteJobWhenFinished);
    void cancelJob(juce::ThreadPoolJob* job);
    juce::ThreadPool& getThreadPool();

private:
    bool showSettingsMenu = false;
    bool recenterButtonActive = false;
    bool isInitialValueApplied = false;

    MurImage m1logo;

    MurkaPoint cachedMousePositionWhenMouseWasHidden = { 0, 0 };
    MurkaPoint currentMousePosition = { 0, 0 };

    std::function<void()> cursorHide = [&]() {
        setMouseCursor(juce::MouseCursor::NoCursor);
        cachedMousePositionWhenMouseWasHidden = currentMousePosition;
    };
    std::function<void()> cursorShow = [&]() {
        setMouseCursor(juce::MouseCursor::NormalCursor);
    };
    std::function<void()> cursorShowAndTeleportBack = [&]() {
        setMouseCursor(juce::MouseCursor::NormalCursor);
        juce::Desktop::setMousePosition(localPointToGlobal(juce::Point<int>(cachedMousePositionWhenMouseWasHidden.x, cachedMousePositionWhenMouseWasHidden.y)));
    };
    std::function<void(int, int)> teleportCursor = [&](int x, int y) {
        //
    };

    M1OrientationClientWindow* orientationControlWindow;
    bool showOrientationControlMenu = false;
    bool showedOrientationControlBefore = false;
    bool showMonitorModeDropdown = false;
    bool isAnyTextfieldActived = false;

    void draw_orientation_client(murka::Murka& m, M1OrientationClient& m1OrientationClient);

    juce::ThreadPool jobThreads{ std::max(4, juce::SystemStats::getNumCpus()) };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MonitorUIBaseComponent)
};
