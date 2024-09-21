#include "MonitorUIBaseComponent.h"

using namespace murka;

//==============================================================================
MonitorUIBaseComponent::MonitorUIBaseComponent(M1MonitorAudioProcessor* processor_)
{
    // Make sure you set the size of the component after
    // you add any child components.
    setSize(getWidth(), getHeight());

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
    m1logo.loadFromRawData(BinaryData::mach1logo_png, BinaryData::mach1logo_pngSize);
}

void MonitorUIBaseComponent::timerCallback()
{
    if (shouldResizeComponent)
    {
        repaint();
        setSize(targetSize.x, targetSize.y);
        editor->setSize(targetSize.x, targetSize.y);
        shouldResizeComponent = false;
    }
}

void MonitorUIBaseComponent::draw_orientation_client(murka::Murka& m, M1OrientationClient& m1OrientationClient)
{
    std::vector<M1OrientationClientWindowDeviceSlot> slots;

    std::vector<M1OrientationDeviceInfo> devices = m1OrientationClient.getDevices();
    for (int i = 0; i < devices.size(); i++)
    {
        std::string icon = "";
        if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeSerial && devices[i].getDeviceName().find("Bluetooth-Incoming-Port") != std::string::npos)
        {
            icon = "bt";
        }
        else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeSerial && devices[i].getDeviceName().find("Mach1-") != std::string::npos)
        {
            icon = "bt";
        }
        else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeBLE)
        {
            icon = "bt";
        }
        else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeSerial)
        {
            icon = "usb";
        }
        else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeCamera)
        {
            icon = "camera";
        }
        else if (devices[i].getDeviceType() == M1OrientationDeviceType::M1OrientationManagerDeviceTypeEmulator)
        {
            icon = "none";
        }
        else
        {
            icon = "wifi";
        }

        std::string name = devices[i].getDeviceName();
        slots.push_back({ icon,
            name,
            name == m1OrientationClient.getCurrentDevice().getDeviceName(),
            i,
            [&](int idx) {
                m1OrientationClient.command_startTrackingUsingDevice(devices[idx]);
            } });
    }

    // trigger a server side refresh for listed devices while menu is open
    m1OrientationClient.command_refresh();
    //bool showOrientationSettingsPanelInsideWindow = (m1OrientationClient.getCurrentDevice().getDeviceType() != M1OrientationManagerDeviceTypeNone);
    orientationControlWindow = &(m.prepare<M1OrientationClientWindow>({ 400, 378, 290, 400 }));
    orientationControlWindow->withDeviceSlots(slots);
    orientationControlWindow->withOrientationClient(m1OrientationClient);
    orientationControlWindow->draw();
}

void MonitorUIBaseComponent::draw()
{
    // This clears the context with our background.
    //juce::OpenGLHelpers::clear(juce::Colour(255.0, 198.0, 30.0));

    float scale = (float)openGLContext.getRenderingScale() * 0.7f; // (Desktop::getInstance().getMainMouseSource().getScreenPosition().x / 300.0); //  0.7;

    if (scale != m.getScreenScale())
    {
        m.setScreenScale(scale);
        m.updateFontsTextures(&m);
        m.clearFontsTextures();
    }

    m.setColor(BACKGROUND_GREY);
    m.clear();

    if (processor->monitorOSC.IsActiveMonitor())
    {
        // Monitor plugin is marked as active, this is used to disable monitor plugin instances when more than 1 is discovered via the OSC messaging

        if (processor->external_spatialmixer_active)
        {
            // External Mixer detected!
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE + 2);

            auto& monitorStateLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() / 2, m.getSize().height() / 2, 200, 80));
            monitorStateLabel.label = "M1-MONITOR DEACTIVATED";
            monitorStateLabel.alignment = TEXT_CENTER;
            monitorStateLabel.enabled = false;
            monitorStateLabel.highlighted = false;
            monitorStateLabel.draw();

            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE - 2);

            // TODO: Add more state messages if needed
            auto& monitorStateDescLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() / 2, m.getSize().height() / 2 - 80, 200, 80));
            monitorStateDescLabel.label = "EXTERNAL MIXER DETECTED";
            monitorStateDescLabel.alignment = TEXT_CENTER;
            monitorStateDescLabel.enabled = false;
            monitorStateDescLabel.highlighted = false;
            monitorStateDescLabel.draw();
        }
        else
        {
            // External Mixer not detected!

            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);
            Mach1::Float3 current_ext_orientation = processor->m1OrientationClient.getOrientation().GetGlobalRotationAsEulerDegrees();

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
            yawRadial.withFontSize(DEFAULT_FONT_SIZE - 3);
            yawRadial.activated = !isAnyTextfieldActived;
            yawRadial.orientationClientConnected = processor->m1OrientationClient.isConnectedToDevice();
            yawRadial.orientationClientValue = current_ext_orientation.GetYaw();
            yawRadial.draw();

            if (yawRadial.changed)
            {
                double normalisedValue = processor->parameters.getParameter(processor->paramYaw)->convertTo0to1(monitorState->yaw);
                processor->parameters.getParameter(processor->paramYaw)->setValueNotifyingHost((float)normalisedValue);
            }

            auto& pitchSlider = m.prepare<M1Slider>({ 465, 45, 160, 140 }).withLabel("PITCH").hasMovingLabel(true).withFontSize(DEFAULT_FONT_SIZE - 3).drawHorizontal(false);
            pitchSlider.cursorHide = cursorHide;
            pitchSlider.cursorShow = cursorShow;
            pitchSlider.rangeFrom = -90.;
            pitchSlider.rangeTo = 90.;
            pitchSlider.defaultValue = 0.0;
            pitchSlider.postfix = "º";
            pitchSlider.dataToControl = &monitorState->pitch;
            pitchSlider.orientationClientConnected = processor->m1OrientationClient.isConnectedToDevice();
            pitchSlider.orientationClientValue = current_ext_orientation.GetPitch();
            if (monitorState->monitor_mode == 2 || monitorState->m1Decode.getFormatChannelCount() <= 4)
            {
                // Disabling pitch slider because we are either a non-spatial review mode or using 4 channel Mach1 Horizon format which only supports yaw rotation playback
                pitchSlider.enabled = false;
            }
            else
            {
                pitchSlider.enabled = true;
            }
            pitchSlider.draw();

            if (pitchSlider.changed)
            {
                double normalisedValue = (processor->parameters.getParameter(processor->paramPitch)->convertTo0to1(monitorState->pitch));
                processor->parameters.getParameter(processor->paramPitch)->setValueNotifyingHost((float)normalisedValue);
            }

            auto& rollSlider = m.prepare<M1Slider>({ 465, 180, 160, 160 }).withLabel("ROLL").hasMovingLabel(true).withFontSize(DEFAULT_FONT_SIZE - 3).drawHorizontal(true);
            rollSlider.cursorHide = cursorHide;
            rollSlider.cursorShow = cursorShow;
            rollSlider.rangeFrom = -90.;
            rollSlider.rangeTo = 90.;
            rollSlider.defaultValue = 0.0;
            rollSlider.postfix = "º";
            rollSlider.dataToControl = &monitorState->roll;
            rollSlider.orientationClientConnected = processor->m1OrientationClient.isConnectedToDevice();
            rollSlider.orientationClientValue = current_ext_orientation.GetRoll();
            if (monitorState->monitor_mode == 2 || monitorState->m1Decode.getFormatChannelCount() <= 4)
            {
                // Disabling roll slider because we are either a non-spatial review mode or using 4 channel Mach1 Horizon format which only supports yaw rotation playback
                rollSlider.enabled = false;
            }
            else
            {
                rollSlider.enabled = true;
            }
            rollSlider.draw();

            if (rollSlider.changed)
            {
                double normalisedValue = (processor->parameters.getParameter(processor->paramRoll)->convertTo0to1(monitorState->roll));
                processor->parameters.getParameter(processor->paramRoll)->setValueNotifyingHost((float)normalisedValue);
            }
        }

        std::function<void()> deleteTheSettingsButton = [&]() {
            // Temporary solution to delete the TextField:
            // Searching for an id to delete the text field widget.
            // To be redone after the UI library refactoring.

            imIdentifier idToDelete;
            for (auto childTuple : m.imChildren)
            {
                auto childIdTuple = childTuple.first;
                if (std::get<1>(childIdTuple) == typeid(M1DropdownButton).name())
                {
                    idToDelete = childIdTuple;
                }
            }
            m.imChildren.erase(idToDelete);
        };

        isAnyTextfieldActived = false;

        if (showSettingsMenu)
        {
            // Settings rendering
            float leftSide_LeftBound_x = 18;
            float rightSide_LeftBound_x = 380;
            float bottomSettings_topBound_y = 382;

            setShouldResizeTo(MurkaPoint(504, 467));

            /// LEFT SIDE

            m.setColor(ENABLED_PARAM);
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE - 2);
            m.prepare<murka::Label>({ leftSide_LeftBound_x + 10, bottomSettings_topBound_y, 150, 20 })
                .withAlignment(TEXT_LEFT)
                .text("MONITOR MODE")
                .draw();

            m.setColor(BACKGROUND_COMPONENT);
            m.enableFill();
            m.drawRectangle(20, bottomSettings_topBound_y + 25, 310, 40);
            m.setColor(ENABLED_PARAM);

            std::vector<std::string> monitorModes = { "MACH1 SPATIAL (DEFAULT)", "STEREO SAFE", "FRONT/BACK FOLDDOWN" };
            auto& modeDropdown = m.prepare<M1DropdownMenu>({ 20, bottomSettings_topBound_y + 25, 310, 120 }).withOptions(monitorModes);
            modeDropdown.fontSize = DEFAULT_FONT_SIZE - 3;
            modeDropdown.withHighlightLabelColor(MurkaColor(BACKGROUND_GREY));
            modeDropdown.textAlignment = TEXT_LEFT;
            modeDropdown.labelPadding_x = 5;
            modeDropdown.optionHeight = 40;

            if (!showMonitorModeDropdown)
            {
                auto& dropdownInit = m.prepare<M1DropdownButton>({ 20, bottomSettings_topBound_y + 25, 310, 40 }).withLabel(monitorModes[monitorState->monitor_mode]).withOutline(true).withBackgroundColor(MurkaColor(BACKGROUND_GREY)).withOutlineColor(MurkaColor(ENABLED_PARAM)).withTriangle(true);
                dropdownInit.textAlignment = TEXT_LEFT;
                dropdownInit.fontSize = DEFAULT_FONT_SIZE - 3;
                dropdownInit.labelPadding_x = 5;
                dropdownInit.draw();

                if (dropdownInit.pressed)
                {
                    showMonitorModeDropdown = true;
                    modeDropdown.open();
                }
            }
            else
            {
                m.addOverlay([&]() {
                    modeDropdown.draw();
                    if (modeDropdown.changed || !modeDropdown.opened)
                    {
                        monitorState->monitor_mode = modeDropdown.selectedOption;
                        double normalisedValue = processor->parameters.getParameter(processor->paramMonitorMode)->convertTo0to1(monitorState->monitor_mode);
                        processor->parameters.getParameter(processor->paramMonitorMode)->setValueNotifyingHost((float)normalisedValue);
                        showMonitorModeDropdown = false;
                        modeDropdown.close();
                    }
                },
                    &modeDropdown);
            }

            //Timecode offset
            m.setColor(ENABLED_PARAM);
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE - 2);
            m.prepare<murka::Label>({ leftSide_LeftBound_x + 10, bottomSettings_topBound_y + 77, 150, 20 })
                .withAlignment(TEXT_LEFT)
                .text("TIMECODE OFFSET")
                .draw();

            m.setColor(BACKGROUND_COMPONENT);
            m.enableFill();
            m.drawRectangle(leftSide_LeftBound_x, bottomSettings_topBound_y + 100, 310, 40);

            m.setColor(ENABLED_PARAM);
            auto& hhfield = m.prepare<murka::TextField>({ leftSide_LeftBound_x + 5, bottomSettings_topBound_y + 105, 30, 30 }).onlyAllowNumbers(true).fillWidthWithZeroes(2).controlling(&processor->transport->HH);
            if (processor->transport->HH < 0)
                processor->transport->HH = 0;
            if (processor->transport->HH > 100)
                processor->transport->HH = 99;
            hhfield.widgetBgColor.setAlpha(0);
            hhfield.drawBounds = false;
            hhfield.shouldSelectAllWhenClicked = true;
            hhfield.draw();

            m.prepare<murka::Label>({ leftSide_LeftBound_x + 38, bottomSettings_topBound_y + 113, 30, 30 }).withAlignment(TEXT_LEFT).text(":").draw();

            auto& mmfield = m.prepare<murka::TextField>({ leftSide_LeftBound_x + 50, bottomSettings_topBound_y + 105, 30, 30 }).onlyAllowNumbers(true).fillWidthWithZeroes(2).controlling(&processor->transport->MM);
            if (processor->transport->MM < 0)
                processor->transport->MM = 0;
            if (processor->transport->MM > 100)
                processor->transport->MM = 99;
            mmfield.widgetBgColor.setAlpha(0);
            mmfield.drawBounds = false;
            mmfield.shouldSelectAllWhenClicked = true;
            mmfield.draw();

            m.prepare<murka::Label>({ leftSide_LeftBound_x + 83, bottomSettings_topBound_y + 113, 30, 30 }).withAlignment(TEXT_LEFT).text(":").draw();

            auto& ssfield = m.prepare<murka::TextField>({ leftSide_LeftBound_x + 95, bottomSettings_topBound_y + 105, 30, 30 }).onlyAllowNumbers(true).fillWidthWithZeroes(2).controlling(&processor->transport->SS);
            if (processor->transport->SS < 0)
                processor->transport->SS = 0;
            if (processor->transport->SS > 100)
                processor->transport->SS = 99;
            ssfield.widgetBgColor.setAlpha(0);
            ssfield.drawBounds = false;
            ssfield.shouldSelectAllWhenClicked = true;
            ssfield.draw();

            m.prepare<murka::Label>({ leftSide_LeftBound_x + 128, bottomSettings_topBound_y + 113, 30, 30 }).withAlignment(TEXT_LEFT).text(":").draw();

            auto& fsfield = m.prepare<murka::TextField>({ leftSide_LeftBound_x + 140, bottomSettings_topBound_y + 105, 30, 30 }).onlyAllowNumbers(true).fillWidthWithZeroes(2).controlling(&processor->transport->FS);
            if (processor->transport->FS < 0)
                processor->transport->FS = 0;
            if (processor->transport->FS > 100)
                processor->transport->FS = 99;
            fsfield.widgetBgColor.setAlpha(0);
            fsfield.drawBounds = false;
            fsfield.shouldSelectAllWhenClicked = true;
            fsfield.draw();

            if (hhfield.boundReached)
            {
                if (hhfield.cursorPosition >= 2)
                {
                    hhfield.cursorPosition = 0;
                    hhfield.deactivate();
                    mmfield.activate();
                    mmfield.shouldForceEditorToSelectAll = true;
                }
            };

            if (mmfield.boundReached)
            {
                if (mmfield.cursorPosition == 0)
                {
                    mmfield.cursorPosition = 0;
                    mmfield.deactivate();
                    hhfield.activate();
                    hhfield.shouldForceEditorToSelectAll = true;
                }
                if (mmfield.cursorPosition == 2)
                {
                    mmfield.cursorPosition = 0;
                    mmfield.deactivate();
                    ssfield.activate();
                    ssfield.shouldForceEditorToSelectAll = true;
                }
            };

            if (ssfield.boundReached)
            {
                if (ssfield.cursorPosition == 0)
                {
                    ssfield.cursorPosition = 0;
                    ssfield.deactivate();
                    mmfield.activate();
                    mmfield.shouldForceEditorToSelectAll = true;
                }
                if (ssfield.cursorPosition >= 2)
                {
                    ssfield.cursorPosition = 0;
                    ssfield.deactivate();
                    fsfield.activate();
                    fsfield.shouldForceEditorToSelectAll = true;
                }
            };

            if (fsfield.boundReached)
            {
                if (fsfield.cursorPosition == 0)
                {
                    fsfield.cursorPosition = 0;
                    fsfield.deactivate();
                    ssfield.activate();
                    ssfield.shouldForceEditorToSelectAll = true;
                }
            };

            isAnyTextfieldActived = hhfield.activated || mmfield.activated || ssfield.activated || fsfield.activated;

            /// RIGHT SIDE

            // orientation client window
            draw_orientation_client(m, processor->m1OrientationClient);

            /// Bottom bar
#ifdef CUSTOM_CHANNEL_LAYOUT
            // Remove bottom bar for CUSTOM_CHANNEL_LAYOUT macro
#else
            int dropdownItemHeight = 20;

            if (!processor->hostType.isProTools() || // is not PT
                (processor->hostType.isProTools() && // or has an output dropdown in PT
                    (processor->getMainBusNumInputChannels() == 8 || processor->getMainBusNumInputChannels() == 16 || processor->getMainBusNumInputChannels() == 36 || processor->getMainBusNumInputChannels() == 64)))
            {
                // Show bottom bar
                m.setLineWidth(1);
                m.setColor(GRID_LINES_3_RGBA);
                m.drawLine(0, m.getSize().height() - 36, m.getSize().width(), m.getSize().height() - 36); // Divider line
                m.setColor(BACKGROUND_GREY);
                m.drawRectangle(0, m.getSize().height(), m.getSize().width(), 35); // bottom bar

                m.setColor(APP_LABEL_TEXT_COLOR);
                m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE - 3);

                // OUTPUT DROPDOWN & LABELS
                /// --> label
                m.setColor(ENABLED_PARAM);
                m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE - 3);
                auto& arrowLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() / 2 - 20, m.getSize().height() - 26, 40, 20));
                arrowLabel.label = "-->";
                arrowLabel.alignment = TEXT_CENTER;
                arrowLabel.enabled = false;
                arrowLabel.highlighted = false;
                arrowLabel.draw();

                // OUTPUT DROPDOWN or LABEL
                m.setColor(ENABLED_PARAM);
                m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE - 3);
                auto& outputLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() / 2 + 105, m.getSize().height() - 26, 60, 20));
                outputLabel.label = "DECODE";
                outputLabel.alignment = TEXT_CENTER;
                outputLabel.enabled = false;
                outputLabel.highlighted = false;
                outputLabel.draw();

                auto& outputDropdownButton = m.prepare<M1DropdownButton>({ m.getSize().width() / 2 + 20, m.getSize().height() - 28, 80, 20 })
                                                 .withLabel(std::to_string(monitorState->m1Decode.getFormatChannelCount()))
                                                 .withOutline(true);
                outputDropdownButton.fontSize = DEFAULT_FONT_SIZE - 5;
                outputDropdownButton.draw();
                std::vector<std::string> output_options = { "M1Horizon-4", "M1Spatial-8" };
                if (processor->hostType.isProTools())
                {
                    // more selective assignment in PT only
                    if (processor->getMainBusNumInputChannels() == 16)
                    {
                        output_options.push_back("M1Spatial-12");
                        output_options.push_back("M1Spatial-14");
                    }
                    else if (processor->getMainBusNumInputChannels() == 36)
                    {
                        output_options.push_back("M1Spatial-12");
                        output_options.push_back("M1Spatial-14");
                    }
                    else if (processor->getMainBusNumInputChannels() == 64)
                    {
                        output_options.push_back("M1Spatial-12");
                        output_options.push_back("M1Spatial-14");
                    }
                }
                else
                {
                    if (processor->external_spatialmixer_active || processor->getMainBusNumInputChannels() >= 12)
                        output_options.push_back("M1Spatial-12");
                    if (processor->external_spatialmixer_active || processor->getMainBusNumInputChannels() >= 14)
                        output_options.push_back("M1Spatial-14");
                }

                auto& outputDropdownMenu = m.prepare<M1DropdownMenu>({ m.getSize().width() / 2 + 20,
                                                                         m.getSize().height() - 28 - output_options.size() * dropdownItemHeight,
                                                                         120,
                                                                         output_options.size() * dropdownItemHeight })
                                               .withOptions(output_options);

                // Mach1Decode API has the first index [0] set to Mach1DecodeAlgoSpatial_8
                // and [1] as Mach1DecodeAlgoSpatial_8 which is the opposite of how we list them in the UI
                // this function swaps them
                if ((int)processor->parameters.getParameter(processor->paramOutputMode)->convertFrom0to1(processor->parameters.getParameter(processor->paramOutputMode)->getValue()) == (int)Mach1DecodeAlgoHorizon_4)
                { /* Swap to Mach1DecodeAlgoSpatial_4 */
                    outputDropdownMenu.selectedOption = 0;
                }
                if ((int)processor->parameters.getParameter(processor->paramOutputMode)->convertFrom0to1(processor->parameters.getParameter(processor->paramOutputMode)->getValue()) == (int)Mach1DecodeAlgoSpatial_8)
                { /* Swap to Mach1DecodeAlgoSpatial_8 */
                    outputDropdownMenu.selectedOption = 1;
                }

                if (outputDropdownButton.pressed)
                {
                    outputDropdownMenu.open();
                }

                outputDropdownMenu.optionHeight = dropdownItemHeight;
                outputDropdownMenu.fontSize = DEFAULT_FONT_SIZE - 5;
                outputDropdownMenu.draw();

                if (outputDropdownMenu.changed)
                {
                    if (outputDropdownMenu.selectedOption == 0)
                    {
                        processor->parameters.getParameter(processor->paramOutputMode)->setValueNotifyingHost(processor->parameters.getParameter(processor->paramOutputMode)->convertTo0to1(Mach1DecodeAlgoHorizon_4));
                    }
                    else if (outputDropdownMenu.selectedOption == 1)
                    {
                        processor->parameters.getParameter(processor->paramOutputMode)->setValueNotifyingHost(processor->parameters.getParameter(processor->paramOutputMode)->convertTo0to1(Mach1DecodeAlgoSpatial_8));
                    }
                    else if (outputDropdownMenu.selectedOption == 2)
                    {
                        processor->parameters.getParameter(processor->paramOutputMode)->setValueNotifyingHost(processor->parameters.getParameter(processor->paramOutputMode)->convertTo0to1(Mach1DecodeAlgoSpatial_12));
                    }
                    else if (outputDropdownMenu.selectedOption == 3)
                    {
                        processor->parameters.getParameter(processor->paramOutputMode)->setValueNotifyingHost(processor->parameters.getParameter(processor->paramOutputMode)->convertTo0to1(Mach1DecodeAlgoSpatial_14));
                    }
                }
            }
            else
            {
                // multichannel PT
                // hide bottom bar
            }
#endif // end of bottom bar macro check
        }
        else
        {
            setShouldResizeTo(MurkaPoint(504, 267));
        }

        /// SETTINGS BUTTON
        m.setColor(ENABLED_PARAM);
        float settings_button_height = 370;
        if (showSettingsMenu)
        {
            auto& showSettingsWhileOpenedButton = m.prepare<M1DropdownButton>({ m.getSize().width() / 2 - 30, settings_button_height - 25, 120, 30 })
                                                      .withLabel("SETTINGS")
                                                      .withFontSize(DEFAULT_FONT_SIZE)
                                                      .withOutline(false);
            showSettingsWhileOpenedButton.textAlignment = TEXT_LEFT;
            showSettingsWhileOpenedButton.draw();

            if (showSettingsWhileOpenedButton.pressed)
            {
                showSettingsMenu = false;
                deleteTheSettingsButton();
            }
        }
        else
        {
            auto& showSettingsWhileClosedButton = m.prepare<M1DropdownButton>({ m.getSize().width() / 2 - 30, settings_button_height - 25, 120, 30 })
                                                      .withLabel("SETTINGS")
                                                      .withFontSize(DEFAULT_FONT_SIZE)
                                                      .withOutline(false);
            showSettingsWhileClosedButton.textAlignment = TEXT_LEFT;
            showSettingsWhileClosedButton.draw();

            if (showSettingsWhileClosedButton.pressed)
            {
                showSettingsMenu = true;
                deleteTheSettingsButton();
            }
        }

        // draw Settings button arrow
        if (showSettingsMenu)
        {
            // draw settings arrow indicator pointing up
            m.enableFill();
            m.setColor(LABEL_TEXT_COLOR);
            MurkaPoint triangleCenter = { m.getSize().width() / 2 + 60, settings_button_height - 7 };
            std::vector<MurkaPoint3D> triangle;
            triangle.push_back({ triangleCenter.x - 5, triangleCenter.y, 0 });
            triangle.push_back({ triangleCenter.x + 5, triangleCenter.y, 0 }); // top middle
            triangle.push_back({ triangleCenter.x, triangleCenter.y - 5, 0 });
            triangle.push_back({ triangleCenter.x - 5, triangleCenter.y, 0 });
            m.drawPath(triangle);
        }
        else
        {
            // draw settings arrow indicator pointing down
            m.enableFill();
            m.setColor(LABEL_TEXT_COLOR);
            MurkaPoint triangleCenter = { m.getSize().width() / 2 + 60, settings_button_height - 7 - 5 };
            std::vector<MurkaPoint3D> triangle;
            triangle.push_back({ triangleCenter.x + 5, triangleCenter.y, 0 });
            triangle.push_back({ triangleCenter.x - 5, triangleCenter.y, 0 }); // bottom middle
            triangle.push_back({ triangleCenter.x, triangleCenter.y + 5, 0 });
            triangle.push_back({ triangleCenter.x + 5, triangleCenter.y, 0 });
            m.drawPath(triangle);
        }
    }
    else
    {
        // The monitor has been marked to be disabled

        /// DISABLED label
        m.setColor(ENABLED_PARAM);
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);

        auto& disabledLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() / 2 - 100, m.getSize().height() / 2, 200, 20));
        disabledLabel.label = "PLUGIN DISABLED";
        disabledLabel.alignment = TEXT_CENTER;
        disabledLabel.enabled = false;
        disabledLabel.highlighted = false;
        disabledLabel.draw();

        /// "Enable this M1-Monitor" button
        m.prepare<M1Label>(MurkaShape(m.getSize().width() / 2 - 150, m.getSize().height() / 2 - 60, 300, 40))
            .withText("ENABLE THIS M1-MONITOR")
            .withTextAlignment(TEXT_CENTER)
            .withVerticalTextOffset(10)
            .withOnClickFlash()
            .withBackgroundFill(MurkaColor(DISABLED_PARAM), MurkaColor(BACKGROUND_GREY))
            .withStrokeBorder(MurkaColor(ENABLED_PARAM))
            .withOnClickCallback([&]() {
                processor->monitorOSC.sendRequestToBecomeActive();
            })
            .draw();
    }

    /// Monitor label
    m.setColor(ENABLED_PARAM);
    m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE - 2);
#ifdef CUSTOM_CHANNEL_LAYOUT
    auto& monitorLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() - 100, m.getSize().height() - 30, 80, 20));
#else
    int labelYOffset;
    if (!processor->hostType.isProTools() || // not pro tools
        (processor->hostType.isProTools() && // or is pro tools with multichannel processing
            processor->getMainBusNumInputChannels() > 2))
    {
        labelYOffset = 26;
    }
    else
    {
        labelYOffset = 30;
    }
    auto& monitorLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() - 100, m.getSize().height() - labelYOffset, 80, 20));
#endif
    monitorLabel.label = "MONITOR";
    monitorLabel.alignment = TEXT_CENTER;
    monitorLabel.enabled = false;
    monitorLabel.highlighted = false;
    monitorLabel.draw();

    m.setColor(ENABLED_PARAM);
#ifdef CUSTOM_CHANNEL_LAYOUT
    m.drawImage(m1logo, 25, m.getSize().height() - 30, 161 / 3, 39 / 3);
#else
    m.drawImage(m1logo, 25, m.getSize().height() - labelYOffset, 161 / 3, 39 / 3);
#endif
}

//==============================================================================
void MonitorUIBaseComponent::paint(juce::Graphics& g)
{
    // This will draw over the top of the openGL background.
}

void MonitorUIBaseComponent::resized()
{
    // This is called when the MonitorUIBaseComponent is resized.
}
