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
    bool showMonitorModeDropdown = false;
    
    static void update_orientation_client_window(murka::Murka &m, M1OrientationOSCClient &m1OrientationOSCClient, M1OrientationClientWindow &orientationControlWindow, bool &showOrientationControlMenu, bool showedOrientationControlBefore) {
        std::vector<M1OrientationClientWindowDeviceSlot> slots;
        
        std::vector<M1OrientationDeviceInfo> devices = m1OrientationOSCClient.getDevices();
        for (int i = 0; i < devices.size(); i++) {
            std::string icon = "";
            if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeSerial && devices[i].getDeviceName().find("Bluetooth-Incoming-Port") != std::string::npos) {
                icon = "bt";
            } else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeSerial && devices[i].getDeviceName().find("Mach1-") != std::string::npos) {
                icon = "bt";
            } else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeBLE) {
                icon = "bt";
            } else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeSerial) {
                icon = "usb";
            } else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeCamera) {
                icon = "camera";
            } else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeEmulator) {
                icon = "none";
            } else {
                icon = "wifi";
            }
            
            std::string name = devices[i].getDeviceName();
            slots.push_back({ icon, name, name == m1OrientationOSCClient.getCurrentDevice().getDeviceName(), i, [&](int idx)
                {
                    m1OrientationOSCClient.command_startTrackingUsingDevice(devices[idx]);
                }
            });
        }
        
        auto& orientationControlButton = m.prepare<M1OrientationWindowToggleButton>({ m.getSize().width() - 40 - 5, 5, 40, 40 }).onClick([&](M1OrientationWindowToggleButton& b) {
            showOrientationControlMenu = !showOrientationControlMenu;
        })
            .withInteractiveOrientationGimmick(m1OrientationOSCClient.getCurrentDevice().getDeviceType() != M1OrientationManagerDeviceTypeNone, m1OrientationOSCClient.getOrientation().getYPR().yaw)
            .draw();
        
        // TODO: move this to be to the left of the orientation client window button
        if (std::holds_alternative<bool>(m1OrientationOSCClient.getCurrentDevice().batteryPercentage)) {
            // it's false, which means the battery percentage is unknown
        } else {
            // it has a battery percentage value
            int battery_value = std::get<int>(m1OrientationOSCClient.getCurrentDevice().batteryPercentage);
            m.getCurrentFont()->drawString("Battery: " + std::to_string(battery_value), m.getWindowWidth() - 100, m.getWindowHeight() - 100);
        }
        
        if (orientationControlButton.hovered && (m1OrientationOSCClient.getCurrentDevice().getDeviceType() != M1OrientationManagerDeviceTypeNone)) {
            std::string deviceReportString = "CONNECTED DEVICE: " + m1OrientationOSCClient.getCurrentDevice().getDeviceName();
            auto font = m.getCurrentFont();
            auto bbox = font->getStringBoundingBox(deviceReportString, 0, 0);
            //m.setColor(40, 40, 40, 200);
            // TODO: fix this bounding box (doesnt draw the same place despite matching settings with Label.draw
            //m.drawRectangle(     m.getSize().width() - 40 - 10 /* padding */ - bbox.width - 5, 5, bbox.width + 10, 40);
            m.setColor(230, 230, 230);
            m.prepare<M1Label>({ m.getSize().width() - 40 - 10 /* padding */ - bbox.width - 5, 5 + 10, bbox.width + 10, 40 }).text(deviceReportString).withTextAlignment(TEXT_CENTER).draw();
        }
        
        if (showOrientationControlMenu) {
            bool showOrientationSettingsPanelInsideWindow = (m1OrientationOSCClient.getCurrentDevice().getDeviceType() != M1OrientationManagerDeviceTypeNone);
            orientationControlWindow = m.prepare<M1OrientationClientWindow>({ m.getSize().width() - 218 - 5 , 5, 218, 300 + 100 * showOrientationSettingsPanelInsideWindow })
                .withDeviceList(slots)
                .withSettingsPanelEnabled(showOrientationSettingsPanelInsideWindow)
                .onClickOutside([&]() {
                    if (!orientationControlButton.hovered) { // Only switch showing the orientation control if we didn't click on the button
                        showOrientationControlMenu = !showOrientationControlMenu;
                        if (showOrientationControlMenu && !showedOrientationControlBefore) {
                            orientationControlWindow.startRefreshing();
                        }
                    }
                })
                .onDisconnectClicked([&]() {
                    m1OrientationOSCClient.command_disconnect();
                })
                .onRefreshClicked([&]() {
                    m1OrientationOSCClient.command_refreshDevices();
                })
                .onYPRSwitchesClicked([&](int whichone) {
                    if (whichone == 0) m1OrientationOSCClient.command_setTrackingYawEnabled(!m1OrientationOSCClient.getTrackingYawEnabled());
                    if (whichone == 1) m1OrientationOSCClient.command_setTrackingPitchEnabled(!m1OrientationOSCClient.getTrackingPitchEnabled());
                    if (whichone == 2) m1OrientationOSCClient.command_setTrackingRollEnabled(!m1OrientationOSCClient.getTrackingRollEnabled());
                })
                .withYPRTrackingSettings(
                                         m1OrientationOSCClient.getTrackingYawEnabled(),
                                         m1OrientationOSCClient.getTrackingPitchEnabled(),
                                         m1OrientationOSCClient.getTrackingRollEnabled(),
                                         std::pair<int, int>(0, 180),
                                         std::pair<int, int>(0, 180),
                                         std::pair<int, int>(0, 180))
                .withYPR(
                         m1OrientationOSCClient.getOrientation().getYPR().yaw,
                         m1OrientationOSCClient.getOrientation().getYPR().pitch,
                         m1OrientationOSCClient.getOrientation().getYPR().roll
                         );

                            orientationControlWindow.draw();
        }
    }

    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MonitorUIBaseComponent)
};
