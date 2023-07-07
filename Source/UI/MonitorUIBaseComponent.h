/*
  ==============================================================================

    This file was auto-generated!

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

#include "juce_murka/JuceMurkaBaseComponent.h"

#include "../TypesForDataExchange.h"
#include "../PluginProcessor.h"
#include "../Config.h"

#include "M1Radial.h"
#include "M1Slider.h"
#include "M1Checkbox.h"
#include "M1DropdownMenu.h"
#include "M1DropdownButton.h"
#include "m1_orientation_client/UI/M1Label.h"
#include "m1_orientation_client/UI/M1OrientationWindowToggleButton.h"
#include "m1_orientation_client/UI/M1OrientationClientWindow.h"

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MonitorUIBaseComponent : public JuceMurkaBaseComponent
, public juce::Timer
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
    void paint (juce::Graphics& g) override;
    void resized() override;

    bool shouldResizeComponent = false;
    MurkaPoint targetSize;
    
    void setShouldResizeTo(MurkaPoint size) {
        if ((getWidth() != size.x) || (getHeight() != size.y)) {
            targetSize = size;
            shouldResizeComponent = true;
        }
    }
    
    // Timer callback for resizing
    void timerCallback() override;

    
private:
    MurkaPoint cachedMousePositionWhenMouseWasHidden = { 0, 0 };
    MurkaPoint currentMousePosition = { 0, 0 };
    
    bool showSettingsMenu = false;
    bool recenterButtonActive = false;
    
	MurImage m1logo;

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
    
    M1OrientationClientWindow orientationControlWindow;
    bool showOrientationControlMenu = false;
    bool showedOrientationControlBefore = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonitorUIBaseComponent)
};
