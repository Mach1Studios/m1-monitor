#include "MonitorUIBaseComponent.h"

using namespace murka;

//==============================================================================
MonitorUIBaseComponent::MonitorUIBaseComponent(M1MonitorAudioProcessor* processor_)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize (getWidth(), getHeight());
    
    processor = processor_;
    monitorState = &processor->monitorSettings;
    
    startTimerHz(60); // Starts the timer to call the timerCallback method at 60 Hz.
}

MonitorUIBaseComponent::~MonitorUIBaseComponent()
{
}

//==============================================================================
void MonitorUIBaseComponent::initialise()
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
    
    m1logo.loadFromRawData(BinaryData::mach1logo_png, BinaryData::mach1logo_pngSize);
}

void MonitorUIBaseComponent::timerCallback()
{
    if (shouldResizeComponent) {
        repaint();
        setSize(targetSize.x, targetSize.y);
        editor->setSize(targetSize.x, targetSize.y);
        shouldResizeComponent = false;
    }
}

void MonitorUIBaseComponent::update_orientation_client_window(murka::Murka &m, M1OrientationClient &m1OrientationClient, M1OrientationClientWindow* orientationControlWindow, bool &showOrientationControlMenu, bool showedOrientationControlBefore) {
    std::vector<M1OrientationClientWindowDeviceSlot> slots;
    
    std::vector<M1OrientationDeviceInfo> devices = m1OrientationClient.getDevices();
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
        slots.push_back({ icon, name, name == m1OrientationClient.getCurrentDevice().getDeviceName(), i, [&](int idx)
            {
                m1OrientationClient.command_startTrackingUsingDevice(devices[idx]);
            }
        });
    }
    
    auto& orientationControlButton = m.prepare<M1OrientationWindowToggleButton>({ m.getSize().width() - 40 - 5, 5, 40, 40 }).onClick([&](M1OrientationWindowToggleButton& b) {
        showOrientationControlMenu = !showOrientationControlMenu;
    })
    .withInteractiveOrientationGimmick(m1OrientationClient.getCurrentDevice().getDeviceType() != M1OrientationManagerDeviceTypeNone, m1OrientationClient.getOrientation().getYPRasDegrees().yaw)
        .draw();
    
    // TODO: move this to be to the left of the orientation client window button
    if (std::holds_alternative<bool>(m1OrientationClient.getCurrentDevice().batteryPercentage)) {
        // it's false, which means the battery percentage is unknown
    } else {
        // it has a battery percentage value
        int battery_value = std::get<int>(m1OrientationClient.getCurrentDevice().batteryPercentage);
        m.getCurrentFont()->drawString("Battery: " + std::to_string(battery_value), m.getWindowWidth() - 100, m.getWindowHeight() - 100);
    }
    
    if (orientationControlButton.hovered && (m1OrientationClient.getCurrentDevice().getDeviceType() != M1OrientationManagerDeviceTypeNone)) {
        std::string deviceReportString = "CONNECTED DEVICE: " + m1OrientationClient.getCurrentDevice().getDeviceName();
        auto font = m.getCurrentFont();
        auto bbox = font->getStringBoundingBox(deviceReportString, 0, 0);
        //m.setColor(40, 40, 40, 200);
        // TODO: fix this bounding box (doesnt draw the same place despite matching settings with Label.draw
        //m.drawRectangle(     m.getSize().width() - 40 - 10 /* padding */ - bbox.width - 5, 5, bbox.width + 10, 40);
        m.setColor(230, 230, 230);
        m.prepare<M1Label>({ m.getSize().width() - 40 - 10 /* padding */ - bbox.width - 5, 5 + 10, bbox.width + 10, 40 }).text(deviceReportString).withTextAlignment(TEXT_CENTER).draw();
    }
    
    if (showOrientationControlMenu) {
        // trigger a server side refresh for listed devices while menu is open
        m1OrientationClient.command_refresh();
        
        bool showOrientationSettingsPanelInsideWindow = (m1OrientationClient.getCurrentDevice().getDeviceType() != M1OrientationManagerDeviceTypeNone);
        orientationControlWindow = &(m.prepare<M1OrientationClientWindow>({ m.getSize().width() - 218 - 5 , 5, 218, 210 + 130 * showOrientationSettingsPanelInsideWindow })
            .withDeviceList(slots)
            .withSettingsPanelEnabled(showOrientationSettingsPanelInsideWindow)
            .withOscSettingsEnabled((m1OrientationClient.getCurrentDevice().getDeviceType() == M1OrientationManagerDeviceTypeOSC))
            .withSupperwareSettingsEnabled(m1OrientationClient.getCurrentDevice().getDeviceName().find("Supperware HT IMU") != std::string::npos)
            .onClickOutside([&]() {
                if (!orientationControlButton.hovered) { // Only switch showing the orientation control if we didn't click on the button
                    showOrientationControlMenu = !showOrientationControlMenu;
                }
            })
            .onOscSettingsChanged([&](int requested_osc_port, std::string requested_osc_msg_address) {
                m1OrientationClient.command_setAdditionalDeviceSettings("osc_add="+requested_osc_msg_address);
                m1OrientationClient.command_setAdditionalDeviceSettings("osc_p="+std::to_string(requested_osc_port));
            })
            .onSupperwareSettingsChanged([&](bool isRightEarChirality) {
                std::string chir_cmd;
                if (isRightEarChirality) {
                    chir_cmd = "1";
                } else {
                    chir_cmd = "0";
                }
                m1OrientationClient.command_setAdditionalDeviceSettings("sw_chir="+chir_cmd);
            })
            .onDisconnectClicked([&]() {
                m1OrientationClient.command_disconnect();
            })
			.onRecenterClicked([&]() {
                m1OrientationClient.command_recenter();
            })
            .onOscSettingsChanged([&](int requested_osc_port, std::string requested_osc_msg_address) {
                m1OrientationClient.command_setAdditionalDeviceSettings("osc_add="+requested_osc_msg_address);
                m1OrientationClient.command_setAdditionalDeviceSettings("osc_p="+std::to_string(requested_osc_port));
            })
            .onYPRSwitchesClicked([&](int whichone) {
                if (whichone == 0)
                    // yaw clicked
                    monitorState->yawActive = !monitorState->yawActive;
                    m1OrientationClient.command_setTrackingYawEnabled(monitorState->yawActive);
                if (whichone == 1)
                    // pitch clicked
                    monitorState->pitchActive = !monitorState->pitchActive;
                    m1OrientationClient.command_setTrackingPitchEnabled(monitorState->pitchActive);
                if (whichone == 2)
                    // roll clicked
                    monitorState->rollActive = !monitorState->rollActive;
                    m1OrientationClient.command_setTrackingRollEnabled(monitorState->rollActive);
            })
            .withYPRTrackingSettings(
                                     m1OrientationClient.getTrackingYawEnabled(),
                                     m1OrientationClient.getTrackingPitchEnabled(),
                                     m1OrientationClient.getTrackingRollEnabled(),
                                     std::pair<int, int>(0, 180),
                                     std::pair<int, int>(0, 180),
                                     std::pair<int, int>(0, 180)
            )
            .withYPR(
                     m1OrientationClient.getOrientation().getYPRasDegrees().yaw,
                     m1OrientationClient.getOrientation().getYPRasDegrees().pitch,
                     m1OrientationClient.getOrientation().getYPRasDegrees().roll
            ));
            orientationControlWindow->draw();
    }
}

void MonitorUIBaseComponent::draw()
{
    // This clears the context with our background.
    //juce::OpenGLHelpers::clear(juce::Colour(255.0, 198.0, 30.0));
    
    float scale = (float)openGLContext.getRenderingScale() * 0.7; // (Desktop::getInstance().getMainMouseSource().getScreenPosition().x / 300.0); //  0.7;

    if (scale != m.getScreenScale()) {
        m.setScreenScale(scale);
        m.updateFontsTextures(&m);
        m.clearFontsTextures();
    }
   
    m.setColor(BACKGROUND_GREY);
    m.clear();
    
    if (processor->monitorOSC.IsActiveMonitor()) {
        // Monitor plugin is marked as active, this is used to disable monitor plugin instances when more than 1 is discovered via the OSC messaging
        
        if (processor->external_spatialmixer_active) {
            // External Mixer detected!
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE+2);
            
            auto& monitorStateLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width()/2, m.getSize().height()/2, 200, 80));
            monitorStateLabel.label = "M1-MONITOR DEACTIVATED";
            monitorStateLabel.alignment = TEXT_CENTER;
            monitorStateLabel.enabled = false;
            monitorStateLabel.highlighted = false;
            monitorStateLabel.draw();
            
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE-2);
            
            // TODO: Add more state messages if needed
            auto& monitorStateDescLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width()/2, m.getSize().height()/2 - 80, 200, 80));
            monitorStateDescLabel.label = "EXTERNAL MIXER DETECTED";
            monitorStateDescLabel.alignment = TEXT_CENTER;
            monitorStateDescLabel.enabled = false;
            monitorStateDescLabel.highlighted = false;
            monitorStateDescLabel.draw();
            
        } else {
            // External Mixer not detected!
            
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);
            
            /// YPR SLIDERS
            auto& yawRadial = m.prepare<M1Radial>({ 50, 33, 270, 270 }).withLabel("YAW");
            yawRadial.cursorHide = cursorHide;
            yawRadial.cursorShow = cursorShow;
            yawRadial.rangeFrom = 0.;
            yawRadial.rangeTo = 360.;
            yawRadial.defaultValue = 0.;
            yawRadial.postfix = "º";
            yawRadial.dataToControl = &monitorState->yaw;
            yawRadial.enabled = true;
            yawRadial.withFontSize(DEFAULT_FONT_SIZE-2);
            yawRadial.draw();
            
            if (yawRadial.changed) {
                double normalisedValue = processor->parameters.getParameter(processor->paramYaw)->convertTo0to1(monitorState->yaw);
                processor->parameters.getParameter(processor->paramYaw)->setValueNotifyingHost(normalisedValue);
            }
            
            auto& pitchSlider = m.prepare<M1Slider>({ 465, 45, 160, 140 }).withLabel("PITCH")
                .hasMovingLabel(true)
                .withFontSize(DEFAULT_FONT_SIZE-2)
                .drawHorizontal(false);
            pitchSlider.cursorHide = cursorHide;
            pitchSlider.cursorShow = cursorShow;
            pitchSlider.rangeFrom = -90.;
            pitchSlider.rangeTo = 90.;
            pitchSlider.defaultValue = 0.0;
            pitchSlider.postfix = "º";
            pitchSlider.dataToControl = &monitorState->pitch;
            if (monitorState->monitor_mode == 2 || monitorState->m1Decode.getFormatChannelCount() <= 4) {
                // Disabling pitch slider because we are either a non-spatial review mode or using 4 channel Mach1 Horizon format which only supports yaw rotation playback
                pitchSlider.enabled = false;
            } else {
                pitchSlider.enabled = true;
            }
            pitchSlider.draw();
            
            if (pitchSlider.changed) {
                double normalisedValue = ( processor->parameters.getParameter(processor->paramPitch)->convertTo0to1(monitorState->pitch));
                processor->parameters.getParameter(processor->paramPitch)->setValueNotifyingHost(normalisedValue);
            }
            
            auto& rollSlider = m.prepare<M1Slider>({ 465, 180, 160, 160 }).withLabel("ROLL")
                .hasMovingLabel(true)
                .withFontSize(DEFAULT_FONT_SIZE-2)
                .drawHorizontal(true);
            rollSlider.cursorHide = cursorHide;
            rollSlider.cursorShow = cursorShow;
            rollSlider.rangeFrom = -90.;
            rollSlider.rangeTo = 90.;
            rollSlider.defaultValue = 0.0;
            rollSlider.postfix = "º";
            rollSlider.dataToControl = &monitorState->roll;
            if (monitorState->monitor_mode == 2 || monitorState->m1Decode.getFormatChannelCount() <= 4) {
                // Disabling roll slider because we are either a non-spatial review mode or using 4 channel Mach1 Horizon format which only supports yaw rotation playback
                rollSlider.enabled = false;
            } else {
                rollSlider.enabled = true;
            }
            rollSlider.draw();
            
            if (rollSlider.changed) {
                double normalisedValue = (processor->parameters.getParameter(processor->paramRoll)->convertTo0to1(monitorState->roll));
                processor->parameters.getParameter(processor->paramRoll)->setValueNotifyingHost(normalisedValue);
            }
        }
        
        std::function<void()> deleteTheSettingsButton = [&]() {
            // Temporary solution to delete the TextField:
            // Searching for an id to delete the text field widget.
            // To be redone after the UI library refactoring.
            
            imIdentifier idToDelete;
            for (auto childTuple: m.imChildren) {
                auto childIdTuple = childTuple.first;
                if (std::get<1>(childIdTuple) == typeid(M1DropdownButton).name()) {
                    idToDelete = childIdTuple;
                }
            }
            m.imChildren.erase(idToDelete);
        };
        
        /// SETTINGS BUTTON
        m.setColor(ENABLED_PARAM);
        float settings_button_height = 370;
        if (showSettingsMenu) {
            auto& showSettingsWhileOpenedButton = m.prepare<M1DropdownButton>({ m.getSize().width()/2 - 30, settings_button_height - 30,
                120, 30 })
            .withLabel("SETTINGS")
            .withFontSize(DEFAULT_FONT_SIZE)
            .withOutline(false);
            showSettingsWhileOpenedButton.textAlignment = TEXT_LEFT;
            showSettingsWhileOpenedButton.heightDivisor = 3;
            showSettingsWhileOpenedButton.draw();
            
            if (showSettingsWhileOpenedButton.pressed) {
                showSettingsMenu = false;
                deleteTheSettingsButton();
            }
        } else {
            auto& showSettingsWhileClosedButton = m.prepare<M1DropdownButton>({ m.getSize().width()/2 - 30, settings_button_height - 30,
                120, 30 })
            .withLabel("SETTINGS")
            .withFontSize(DEFAULT_FONT_SIZE)
            .withOutline(false);
            showSettingsWhileClosedButton.textAlignment = TEXT_LEFT;
            showSettingsWhileClosedButton.heightDivisor = 3;
            showSettingsWhileClosedButton.draw();
            
            if (showSettingsWhileClosedButton.pressed) {
                showSettingsMenu = true;
                deleteTheSettingsButton();
            }
        }
        
        // draw Settings button arrow
        if (showSettingsMenu) {
            // draw settings arrow indicator pointing up
            m.enableFill();
            m.setColor(LABEL_TEXT_COLOR);
            MurkaPoint triangleCenter = {m.getSize().width()/2 + 65, settings_button_height - 8};
            std::vector<MurkaPoint3D> triangle;
            triangle.push_back({triangleCenter.x - 5, triangleCenter.y, 0});
            triangle.push_back({triangleCenter.x + 5, triangleCenter.y, 0}); // top middle
            triangle.push_back({triangleCenter.x , triangleCenter.y - 5, 0});
            triangle.push_back({triangleCenter.x - 5, triangleCenter.y, 0});
            m.drawPath(triangle);
        } else {
            // draw settings arrow indicator pointing down
            m.enableFill();
            m.setColor(LABEL_TEXT_COLOR);
            MurkaPoint triangleCenter = {m.getSize().width()/2 + 65, settings_button_height - 8 - 5};
            std::vector<MurkaPoint3D> triangle;
            triangle.push_back({triangleCenter.x + 5, triangleCenter.y, 0});
            triangle.push_back({triangleCenter.x - 5, triangleCenter.y, 0}); // bottom middle
            triangle.push_back({triangleCenter.x , triangleCenter.y + 5, 0});
            triangle.push_back({triangleCenter.x + 5, triangleCenter.y, 0});
            m.drawPath(triangle);
        }
        
        if (showSettingsMenu) {
            // Settings rendering
            float leftSide_LeftBound_x = 18;
            float rightSide_LeftBound_x = 380;
            float bottomSettings_topBound_y = 380;
            
            setShouldResizeTo(MurkaPoint(504, 467));
            
            /// LEFT SIDE
            m.setColor(ENABLED_PARAM);
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE-1);
            m.prepare<murka::Label>({leftSide_LeftBound_x, bottomSettings_topBound_y, 150, 20})
                .withAlignment(TEXT_LEFT).text("MONITOR MODE")
                .draw();
            
            m.setColor(BACKGROUND_COMPONENT);
            m.enableFill();
            m.drawRectangle(20, bottomSettings_topBound_y + 20, 310, 40);
            m.setColor(ENABLED_PARAM);
            
            std::vector<std::string> monitorModes = {"MACH1 SPATIAL (DEFAULT)", "STEREO SAFE", "FRONT/BACK FOLDDOWN"};
            auto& modeDropdown = m.prepare<M1DropdownMenu>({20, bottomSettings_topBound_y + 20, 310, 120}).withOptions(monitorModes);
            modeDropdown.textAlignment = TEXT_LEFT;
            modeDropdown.optionHeight = 40;
            
            if (!showMonitorModeDropdown) {
                auto& dropdownInit = m.prepare<M1DropdownButton>({20, bottomSettings_topBound_y + 20, 310, 40}).withLabel(monitorModes[monitorState->monitor_mode]).withOutline(true);
                dropdownInit.textAlignment = TEXT_LEFT;
                dropdownInit.heightDivisor = 3;
                dropdownInit.draw();
                
                if (dropdownInit.pressed) {
                    showMonitorModeDropdown = true;
                    modeDropdown.open();
                }
            } else {
                modeDropdown.draw();
                if (modeDropdown.changed || !modeDropdown.opened) {
                    monitorState->monitor_mode = modeDropdown.selectedOption;
                    double normalisedValue = processor->parameters.getParameter(processor->paramMonitorMode)->convertTo0to1(monitorState->monitor_mode);
                    processor->parameters.getParameter(processor->paramMonitorMode)->setValueNotifyingHost(normalisedValue);
                    showMonitorModeDropdown = false;
                    modeDropdown.close();
                }
            }
            
            /// RIGHT SIDE
            //Broadcast rect
            m.setColor(DISABLED_PARAM);
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE-1);
            m.prepare<murka::Label>({rightSide_LeftBound_x + 2, bottomSettings_topBound_y, 150, 20}).withAlignment(TEXT_LEFT).text("BROADCAST MIX").draw();
            m.setColor(BACKGROUND_COMPONENT);
            m.enableFill();
            m.drawRectangle(rightSide_LeftBound_x, bottomSettings_topBound_y + 20, 310, 40);
            
            //Timecode rect
            m.setColor(GRID_LINES_1_RGBA);
            m.setColor(ENABLED_PARAM);
            m.prepare<murka::Label>({rightSide_LeftBound_x, bottomSettings_topBound_y + 77, 150, 20}).withAlignment(TEXT_LEFT).text("TIME CODE OFFSET").draw();
            m.setColor(BACKGROUND_COMPONENT);
            m.enableFill();
            m.drawRectangle(rightSide_LeftBound_x, bottomSettings_topBound_y + 100, 310, 40);
            
            m.setColor(ENABLED_PARAM);
            auto& hhfield = m.prepare<murka::TextField>({rightSide_LeftBound_x + 5, bottomSettings_topBound_y + 105, 30, 30}).onlyAllowNumbers(true).fillWidthWithZeroes(2).controlling(&processor->transport->HH);
            hhfield.widgetBgColor.a = 0;
            hhfield.drawBounds = false;
            hhfield.draw();
            if (processor->transport->HH < 0) processor->transport->HH = 0;
            if (processor->transport->HH > 100) processor->transport->HH = 99;
            
            m.prepare<murka::Label>({rightSide_LeftBound_x + 35, bottomSettings_topBound_y + 113, 30, 30}).withAlignment(TEXT_LEFT).text(":").draw();
            
            auto& mmfield = m.prepare<murka::TextField>({rightSide_LeftBound_x + 50, bottomSettings_topBound_y + 105, 30, 30}).onlyAllowNumbers(true).fillWidthWithZeroes(2).controlling(&processor->transport->MM);
            if (processor->transport->MM < 0) processor->transport->MM = 0;
            if (processor->transport->MM > 100) processor->transport->MM = 99;
            mmfield.widgetBgColor.a = 0;
            mmfield.drawBounds = false;
            mmfield.draw();
            
            m.prepare<murka::Label>({rightSide_LeftBound_x + 80, bottomSettings_topBound_y + 113, 30, 30}).withAlignment(TEXT_LEFT).text(":").draw();
            
            auto& ssfield = m.prepare<murka::TextField>({rightSide_LeftBound_x + 95, bottomSettings_topBound_y + 105, 30, 30}).onlyAllowNumbers(true).fillWidthWithZeroes(2).controlling(&processor->transport->SS);
            if (processor->transport->SS < 0) processor->transport->SS = 0;
            if (processor->transport->SS > 100) processor->transport->SS = 99;
            ssfield.widgetBgColor.a = 0;
            ssfield.drawBounds = false;
            ssfield.draw();
            
            m.prepare<murka::Label>({rightSide_LeftBound_x + 125, bottomSettings_topBound_y + 113, 30, 30}).withAlignment(TEXT_LEFT).text(":").draw();
            
            auto& fsfield = m.prepare<murka::TextField>({rightSide_LeftBound_x + 140, bottomSettings_topBound_y + 105, 30, 30}).onlyAllowNumbers(true).fillWidthWithZeroes(2).controlling(&processor->transport->FS);
            if (processor->transport->FS < 0) processor->transport->FS = 0;
            if (processor->transport->FS > 100) processor->transport->FS = 99;
            fsfield.widgetBgColor.a = 0;
            fsfield.drawBounds = false;
            fsfield.draw();
            
            /// Bottom bar
#ifdef CUSTOM_CHANNEL_LAYOUT
            // Remove bottom bar for CUSTOM_CHANNEL_LAYOUT macro
#else
            
            int dropdownItemHeight = 20;

            if (!processor->hostType.isProTools() || // is not PT
                (processor->hostType.isProTools() && // or has an output dropdown in PT
                 (processor->getMainBusNumInputChannels() == 8 || processor->getMainBusNumInputChannels() == 16 || processor->getMainBusNumInputChannels() == 36 || processor->getMainBusNumInputChannels() == 64))) {
                
                // Show bottom bar
                m.setLineWidth(1);
                m.setColor(GRID_LINES_3_RGBA);
                m.drawLine(0, m.getSize().height()-36, m.getSize().width(), m.getSize().height()-36); // Divider line
                m.setColor(BACKGROUND_GREY);
                m.drawRectangle(0, m.getSize().height(), m.getSize().width(), 35); // bottom bar
                
                m.setColor(APP_LABEL_TEXT_COLOR);
                m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);
                
                // OUTPUT DROPDOWN & LABELS
                /// --> label
                m.setColor(200, 255);
                m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);
                auto& arrowLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width()/2 - 20, m.getSize().height() - 26, 40, 20));
                arrowLabel.label = "-->";
                arrowLabel.alignment = TEXT_CENTER;
                arrowLabel.enabled = false;
                arrowLabel.highlighted = false;
                arrowLabel.draw();
                
                // OUTPUT DROPDOWN or LABEL
                m.setColor(200, 255);
                m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);
                auto& outputLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width()/2 + 110, m.getSize().height() - 26, 60, 20));
                outputLabel.label = "OUTPUT";
                outputLabel.alignment = TEXT_CENTER;
                outputLabel.enabled = false;
                outputLabel.highlighted = false;
                outputLabel.draw();
                
                auto& outputDropdownButton = m.prepare<M1DropdownButton>({ m.getSize().width()/2 + 20, m.getSize().height() - 28, 80, 20 })
                    .withLabel(std::to_string(monitorState->m1Decode.getFormatChannelCount()))
                    .withOutline(true);
                outputDropdownButton.heightDivisor = 8;
                outputDropdownButton.draw();
                std::vector<std::string> output_options = {"M1Horizon-4", "M1Spatial-8"};
                if (processor->hostType.isProTools()) {
                    // more selective assignment in PT only
                    if (processor->getMainBusNumInputChannels() == 16) {
                        output_options.push_back("M1Spatial-12");
                        output_options.push_back("M1Spatial-14");
                    } else if (processor->getMainBusNumInputChannels() == 36) {
                        output_options.push_back("M1Spatial-12");
                        output_options.push_back("M1Spatial-14");
                        output_options.push_back("M1Spatial-32");
                        output_options.push_back("M1Spatial-36");
                    } else if (processor->getMainBusNumInputChannels() == 64) {
                        output_options.push_back("M1Spatial-12");
                        output_options.push_back("M1Spatial-14");
                        output_options.push_back("M1Spatial-32");
                        output_options.push_back("M1Spatial-36");
                        output_options.push_back("M1Spatial-48");
                        output_options.push_back("M1Spatial-60");
                    }
                } else {
                    if (processor->external_spatialmixer_active || processor->getMainBusNumInputChannels() >= 12) output_options.push_back("M1Spatial-12");
                    if (processor->external_spatialmixer_active || processor->getMainBusNumInputChannels() >= 14) output_options.push_back("M1Spatial-14");
                    if (processor->external_spatialmixer_active || processor->getMainBusNumInputChannels() >= 32) output_options.push_back("M1Spatial-32");
                    if (processor->external_spatialmixer_active || processor->getMainBusNumInputChannels() >= 36) output_options.push_back("M1Spatial-36");
                    if (processor->external_spatialmixer_active || processor->getMainBusNumInputChannels() >= 48) output_options.push_back("M1Spatial-48");
                    if (processor->external_spatialmixer_active || processor->getMainBusNumInputChannels() >= 60) output_options.push_back("M1Spatial-60");
                }
                
                auto& outputDropdownMenu = m.prepare<M1DropdownMenu>({  m.getSize().width()/2 + 20,
                    m.getSize().height() - 28 - output_options.size() * dropdownItemHeight,
                    120, output_options.size() * dropdownItemHeight })
                .withOptions(output_options);
                if (outputDropdownButton.pressed) {
                    outputDropdownMenu.open();
                }
                
                outputDropdownMenu.optionHeight = dropdownItemHeight;
                outputDropdownMenu.fontSize = 9;
                outputDropdownMenu.draw();
                
                if (outputDropdownMenu.changed) {
                    if (outputDropdownMenu.selectedOption == 0) {
                        processor->m1DecodeChangeInputMode(Mach1DecodeAlgoHorizon_4);
                    } else if (outputDropdownMenu.selectedOption == 1) {
                        processor->m1DecodeChangeInputMode(Mach1DecodeAlgoSpatial_8);
                    } else if (outputDropdownMenu.selectedOption == 2) {
                        processor->m1DecodeChangeInputMode(Mach1DecodeAlgoSpatial_12);
                    } else if (outputDropdownMenu.selectedOption == 3) {
                        processor->m1DecodeChangeInputMode(Mach1DecodeAlgoSpatial_14);
                    } else if (outputDropdownMenu.selectedOption == 4) {
                        processor->m1DecodeChangeInputMode(Mach1DecodeAlgoSpatial_32);
                    } else if (outputDropdownMenu.selectedOption == 5) {
                        processor->m1DecodeChangeInputMode(Mach1DecodeAlgoSpatial_36);
                    } else if (outputDropdownMenu.selectedOption == 6) {
                        processor->m1DecodeChangeInputMode(Mach1DecodeAlgoSpatial_48);
                    } else if (outputDropdownMenu.selectedOption == 7) {
                        processor->m1DecodeChangeInputMode(Mach1DecodeAlgoSpatial_60);
                    }
                }
            } else {
                // multichannel PT
                // hide bottom bar
            }
#endif // end of bottom bar macro check
        } else {
            setShouldResizeTo(MurkaPoint(504, 267));
        }
        
        // orientation button
        update_orientation_client_window(m, processor->m1OrientationClient, orientationControlWindow, showOrientationControlMenu, showedOrientationControlBefore);
        
    } else {
        // The monitor has been marked to be disabled
        
        /// DISABLED label
        m.setColor(200, 255);
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);

        auto& disabledLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width()/2 - 100, m.getSize().height()/2, 200, 20));
        disabledLabel.label = "PLUGIN DISABLED";
        disabledLabel.alignment = TEXT_CENTER;
        disabledLabel.enabled = false;
        disabledLabel.highlighted = false;
        disabledLabel.draw();
    }

    /// Monitor label
    m.setColor(200, 255);
    m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);
#ifdef CUSTOM_CHANNEL_LAYOUT
    auto& monitorLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() - 100, m.getSize().height() - 30, 80, 20));
#else
    int labelYOffset;
    if (!processor->hostType.isProTools() || // not pro tools
        (processor->hostType.isProTools() && // or is pro tools with multichannel processing
         processor->getMainBusNumInputChannels() > 2)) {
        labelYOffset = 26;
    } else {
        labelYOffset = 30;
    }
    auto& monitorLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() - 100, m.getSize().height() - labelYOffset, 80, 20));
#endif
    monitorLabel.label = "MONITOR";
    monitorLabel.alignment = TEXT_CENTER;
    monitorLabel.enabled = false;
    monitorLabel.highlighted = false;
    monitorLabel.draw();
    
    m.setColor(200, 255);
#ifdef CUSTOM_CHANNEL_LAYOUT
    m.drawImage(m1logo, 25, m.getSize().height() - 30, 161 / 3, 39 / 3);
#else
    m.drawImage(m1logo, 25, m.getSize().height() - labelYOffset, 161 / 3, 39 / 3);
#endif
}

//==============================================================================
void MonitorUIBaseComponent::paint (juce::Graphics& g)
{
    // You can add your component specific drawing code here!
    // This will draw over the top of the openGL background.
}

void MonitorUIBaseComponent::resized()
{
    // This is called when the MonitorUIBaseComponent is resized.
    // If you add any child components, this is where you should
    // update their positions.
}
