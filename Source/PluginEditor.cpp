/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
M1MonitorAudioProcessorEditor::M1MonitorAudioProcessorEditor (M1MonitorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    juce::OpenGLAppComponent::setSize(504, 266);
    // size when advanced settings open = 504, 469
}

M1MonitorAudioProcessorEditor::~M1MonitorAudioProcessorEditor()
{
}

//==============================================================================
void M1MonitorAudioProcessorEditor::initialise()
{
    JuceMurkaBaseComponent::initialise();

    std::string resourcesPath;
    if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) != 0) {
        resourcesPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory).getChildFile("Application Support/Mach1 Spatial System/resources").getFullPathName().toStdString();
    } else {
        resourcesPath = juce::File::getSpecialLocation(juce::File::SpecialLocationType::userApplicationDataDirectory).getChildFile("Mach1 Spatial System/resources").getFullPathName().toStdString();
    }
    printf("Resources Loaded From: %s \n" , resourcesPath.c_str());
    m.setResourcesPath(resourcesPath);
    
    /// ORIENTATION CONTROL DEBUG
    showedOrientationControlBefore = false;
}

void M1MonitorAudioProcessorEditor::render()
{
    m.begin();
    m.setFont("Proxima Nova Reg.ttf", 10);
    m.setColor(40, 40, 40, 255);
    m.clear();
    
    std::vector<M1OrientationClientWindowDeviceSlot> slots;
    
    slots.push_back({"bt", "bluetooth device 1", 0 == DEBUG_orientationDeviceSelected, 0, [&](int idx)
        {
            DEBUG_orientationDeviceSelected = 0;
        }
    });
    slots.push_back({"bt", "bluetooth device 2", 1 == DEBUG_orientationDeviceSelected, 1, [&](int idx)
        {
           DEBUG_orientationDeviceSelected = 1;
        }
    });
    slots.push_back({"bt", "bluetooth device 3", 2 == DEBUG_orientationDeviceSelected, 2, [&](int idx)
        {
            DEBUG_orientationDeviceSelected = 2;
        }
    });
    slots.push_back({"bt", "bluetooth device 4", 3 == DEBUG_orientationDeviceSelected, 3, [&](int idx)
        {
            DEBUG_orientationDeviceSelected = 3;
        }
    });
    slots.push_back({"wifi", "osc device 1", 4 == DEBUG_orientationDeviceSelected, 4, [&](int idx)
        {
            DEBUG_orientationDeviceSelected = 4;
        }
    });
    slots.push_back({"wifi", "osc device 2", 5 == DEBUG_orientationDeviceSelected, 5, [&](int idx)
        {
            DEBUG_orientationDeviceSelected = 5;
        }
    });
    slots.push_back({"wifi", "osc device 3", 6 == DEBUG_orientationDeviceSelected, 6, [&](int idx)
        {
            DEBUG_orientationDeviceSelected = 6;
        }
    });
    slots.push_back({"wifi", "osc device 4", 7 == DEBUG_orientationDeviceSelected, 7, [&](int idx)
        {
            DEBUG_orientationDeviceSelected = 7;
        }
    });
    
    auto& orientationControlButton = m.draw<M1OrientationWindowToggleButton>({678, 5, 40, 40}).onClick([&](M1OrientationWindowToggleButton& b){
        showOrientationControlMenu = !showOrientationControlMenu;
    })
        .withInteractiveOrientationGimmick(DEBUG_orientationDeviceSelected >= 0, m.getElapsedTime() * 100)
        .commit();
    
    if (orientationControlButton.hovered && (DEBUG_orientationDeviceSelected >= 0)) {
        m.setFont("Proxima Nova Reg.ttf", 12);
        std::string deviceReportString = "Tracking device:" + slots[DEBUG_orientationDeviceSelected].deviceName;
        auto font = m.getCurrentFont();
        auto bbox = font->getStringBoundingBox(deviceReportString, 0, 0);
        m.setColor(40, 40, 40, 200);
        m.drawRectangle(678 + 40 - bbox.width - 5, 45, bbox.width + 10, 30);
        m.setColor(230, 230, 230);
        m.draw<M1Label>({678 + 40 - bbox.width - 5, 48, bbox.width + 10, 30}).text(deviceReportString).commit();
    }
    
    if (showOrientationControlMenu) {
        bool showOrientationSettingsPanelInsideWindow = (DEBUG_orientationDeviceSelected >= 0);
        orientationControlWindow = m.draw<M1OrientationClientWindow>({500, 45, 218, 300 + 100 * showOrientationSettingsPanelInsideWindow}).withDeviceList(slots)
            .withSettingsPanelEnabled(showOrientationSettingsPanelInsideWindow)
            .onClickOutside([&]() {
                if (!orientationControlButton.hovered) { // Only switch showing the orientation control if we didn't click on the button
                    showOrientationControlMenu = !showOrientationControlMenu;
                    if (showOrientationControlMenu && !showedOrientationControlBefore) {
                        orientationControlWindow.startRefreshing();
                    }
                }
            })
            .onDisconnectClicked([&](){
                std::cout << "Now disconnect from the device";
                DEBUG_orientationDeviceSelected = -1;
            })
            .onYPRSwitchesClicked([&](int whichone){
                if (whichone == 0) DEBUG_trackYaw = !DEBUG_trackYaw;
                if (whichone == 1) DEBUG_trackPitch = !DEBUG_trackPitch;
                if (whichone == 2) DEBUG_trackRoll = !DEBUG_trackRoll;
            })
            .withYPRTrackingSettings(DEBUG_trackYaw,
                                     DEBUG_trackPitch,
                                     DEBUG_trackRoll, std::pair<int, int>(0, 180),
                                     std::pair<int, int>(0, 180),
                                     std::pair<int, int>(0, 180));

        orientationControlWindow.commit();
    }
    
    m.setColor(200, 255);
    m.setFont("Proxima Nova Reg.ttf", 10);
    auto& pannerLabel = m.draw<M1Label>(MurkaShape(m.getSize().width() - 100, m.getSize().height() - 30, 80, 20));
    pannerLabel.label = "MONITOR";
    pannerLabel.alignment = TEXT_CENTER;
    pannerLabel.enabled = false;
    pannerLabel.highlighted = false;
    pannerLabel.commit();
    
    m1logo.loadFromRawData(BinaryData::mach1logo_png, BinaryData::mach1logo_pngSize);
    m.drawImage(m1logo, 20, m.getSize().height() - 30, 161 / 3, 39 / 3);
    
    m.end();
}

void M1MonitorAudioProcessorEditor::paint (juce::Graphics& g)
{
}

void M1MonitorAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..
}
