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
}

void MonitorUIBaseComponent::timerCallback() {
    if (shouldResizeComponent) {
        repaint();
        setSize(targetSize.x, targetSize.y);
        editor->setSize(targetSize.x, targetSize.y);
        shouldResizeComponent = false;
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
    
    // TODO: window resize for settings view
    if (showSettingsMenu) {
        // Settings rendering
        setShouldResizeTo(MurkaPoint(504, 330));

        //Timecode rect
        m.setColor(GRID_LINES_1_RGBA);
        m.enableFill();
        m.drawRectangle(265, 327, 215, 27);
        
        //Broadcast rect
        m.enableFill();
        m.drawRectangle(265, 272, 215, 27);
        
        //right side
        m.setColor(DISABLED_PARAM);
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);
        m.prepare<murka::Label>({267, 252, 150, 20}).withAlignment(TEXT_LEFT).text("BROADCAST MIX").draw();
        m.setColor(ENABLED_PARAM);
        m.prepare<murka::Label>({267, 305, 150, 20}).withAlignment(TEXT_LEFT).text("TIME CODE OFFSET").draw();
        auto& hhfield = m.prepare<murka::TextField>({270, 323, 30, 30}).onlyAllowNumbers(true).controlling(&processor->transport->HH);
        hhfield.widgetBgColor.a = 0;
        hhfield.draw();
        if (processor->transport->HH < 0) processor->transport->HH = 0;
        if (processor->transport->HH > 100) processor->transport->HH = 99;
        
        auto& mmfield = m.prepare<murka::TextField>({315, 323, 30, 30}).onlyAllowNumbers(true).controlling(&processor->transport->MM);
        if (processor->transport->MM < 0) processor->transport->MM = 0;
        if (processor->transport->MM > 100) processor->transport->MM = 99;
        mmfield.widgetBgColor.a = 0;
        mmfield.draw();
        
        auto& ssfield = m.prepare<murka::TextField>({360, 323, 30, 30}).onlyAllowNumbers(true).controlling(&processor->transport->SS);
        if (processor->transport->SS < 0) processor->transport->SS = 0;
        if (processor->transport->SS > 100) processor->transport->SS = 99;
        ssfield.widgetBgColor.a = 0;
        ssfield.draw();
        
        auto& fsfield = m.prepare<murka::TextField>({405, 323, 30, 30}).onlyAllowNumbers(true).controlling(&processor->transport->FS);
        if (processor->transport->FS < 0) processor->transport->FS = 0;
        if (processor->transport->FS > 100) processor->transport->FS = 99;
        fsfield.widgetBgColor.a = 0;
        fsfield.draw();
        
        
        m.prepare<murka::Label>({267, 365, 150, 20}).withAlignment(TEXT_LEFT).text("OSC PORT").draw();
        m.prepare<murka::Label>({267, 390, 150, 20}).withAlignment(TEXT_LEFT).text("INPUT").draw();
        
        //left side
        m.setColor(ENABLED_PARAM);
        m.prepare<murka::Label>({17, 252, 150, 20}).withAlignment(TEXT_LEFT).text("MONITOR MODE").draw();
        
        /// Bottom bar
        #ifdef CUSTOM_CHANNEL_LAYOUT
            // Remove bottom bar for CUSTOM_CHANNEL_LAYOUT macro
        #else

        int dropdownItemHeight = 25;

        if (!processor->hostType.isProTools() || (processor->hostType.isProTools() && (processor->getMainBusNumInputChannels() <= 2 || processor->getMainBusNumOutputChannels() <= 2))) {

            // Show bottom bar
            m.setColor(GRID_LINES_3_RGBA);
            m.drawLine(0, m.getSize().height()-36, m.getSize().width(), m.getSize().height()-36); // Divider line
            m.setColor(BACKGROUND_GREY);
            m.drawRectangle(0, m.getSize().height(), m.getSize().width(), 35); // bottom bar
            
            m.setColor(APP_LABEL_TEXT_COLOR);
            m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);

            // OUTPUT DROPDOWN & LABELS
            /// -> label
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
            
            auto& outputDropdownButton = m.prepare<M1DropdownButton>({ m.getSize().width()/2 + 20, m.getSize().height()-33,
                                                        80, 30 })
                                                        .withLabel(std::to_string(monitorState->m1Decode.getFormatChannelCount())).draw();
            std::vector<std::string> output_options = {"M1Horizon-4", "M1Spatial-8"};
            if (processor->external_spatialmixer_active || processor->getMainBusNumOutputChannels() >= 12) output_options.push_back("M1Spatial-12");
            if (processor->external_spatialmixer_active || processor->getMainBusNumOutputChannels() >= 14) output_options.push_back("M1Spatial-14");
            if (processor->external_spatialmixer_active || processor->getMainBusNumOutputChannels() >= 32) output_options.push_back("M1Spatial-32");
            if (processor->external_spatialmixer_active || processor->getMainBusNumOutputChannels() >= 36) output_options.push_back("M1Spatial-36");
            if (processor->external_spatialmixer_active || processor->getMainBusNumOutputChannels() >= 48) output_options.push_back("M1Spatial-48");
            if (processor->external_spatialmixer_active || processor->getMainBusNumOutputChannels() >= 60) output_options.push_back("M1Spatial-60");

            auto& outputDropdownMenu = m.prepare<M1DropdownMenu>({  m.getSize().width()/2 + 20,
                                                                m.getSize().height() - 33 - output_options.size() * dropdownItemHeight,
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
        setShouldResizeTo(MurkaPoint(504, 180));
    }
    
    if (processor->external_spatialmixer_active) {
        // External Mixer detected!
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE+2);

        auto& monitorStateLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width()/2, m.getSize().height()/2, 200, 80));
        monitorStateLabel.label = "M1-MONITOR DEACTIVATED";
        monitorStateLabel.alignment = TEXT_CENTER;
        monitorStateLabel.enabled = false;
        monitorStateLabel.highlighted = false;
        monitorStateLabel.draw();
        
    auto& rollSlider = m.prepare<M1Slider>({   317, 148, 129, 68 })
//                                            .withLabel("ROLL")
                                            .hasMovingLabel(true)
                                            .drawHorizontal(true);
    rollSlider.cursorHide = cursorHide;
    rollSlider.cursorShow = cursorShow;
    rollSlider.rangeFrom = -90.;
    rollSlider.rangeTo = 90.;
    rollSlider.dataToControl = &monitorState->roll;
    rollSlider.enabled = true;
    rollSlider.draw();
    
    if (rollSlider.changed) {
        double normalisedValue = (processor->parameters.getParameter(processor->paramRoll)->convertTo0to1(monitorState->roll) - 0.5 ) / 2;
		processor->parameters.getParameter(processor->paramRoll)->setValueNotifyingHost(normalisedValue);
    }
    
    m.prepare<M1Label>({355, 150, 70, 50}).text("ROLL").draw();
    
	/// CHECKBOXES
    
	float checkboxSlotHeight = 30;
    
    auto& yawActive = m.prepare<M1Checkbox>({ 270, 420,
                                                100, 20 })
                                                .controlling(&monitorState->yawActive)
                                                .withLabel("Y");
    yawActive.enabled = true;
    yawActive.draw();

    if (yawActive.changed) {
        processor->parameterChanged(processor->paramYawEnable, monitorState->yawActive);
    }
    
    auto& pitchActive = m.prepare<M1Checkbox>({ 320, 420,
                                                100, 20 })
                                                .controlling(&monitorState->pitchActive)
                                                .withLabel("P");
    pitchActive.enabled = true;
    pitchActive.draw();

    if (pitchActive.changed) {
        processor->parameterChanged(processor->paramPitchEnable, monitorState->pitchActive);
    }
    
    auto& rollActive = m.prepare<M1Checkbox>({ 370, 420,
                                                100, 20 })
                                                .controlling(&monitorState->rollActive)
                                                .withLabel("R");
    rollActive.enabled = true;
    rollActive.draw();

    if (rollActive.changed) {
        processor->parameterChanged(processor->paramRollEnable, monitorState->rollActive);
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
        auto& yawRadial = m.prepare<M1Radial>({ 35, 25, 189, 189 }).withLabel("YAW");
        yawRadial.cursorHide = cursorHide;
        yawRadial.cursorShow = cursorShow;
        yawRadial.rangeFrom = 0.;
        yawRadial.rangeTo = 360.;
        yawRadial.dataToControl = &monitorState->yaw;
        yawRadial.enabled = true;
        yawRadial.withFontSize(DEFAULT_FONT_SIZE);
        yawRadial.draw();
        
        if (yawRadial.changed) {
            double normalisedValue = processor->parameters.getParameter(processor->paramYaw)->convertTo0to1(monitorState->yaw - 180);
            processor->parameters.getParameter(processor->paramYaw)->setValueNotifyingHost(normalisedValue);
        }
        
        auto& pitchSlider = m.prepare<M1Slider>({  327, 26, 108, 108 })
        //                                            .withLabel("PITCH")
            .hasMovingLabel(true)
            .drawHorizontal(false);
        pitchSlider.cursorHide = cursorHide;
        pitchSlider.cursorShow = cursorShow;
        pitchSlider.rangeFrom = -90.;
        pitchSlider.rangeTo = 90.;
        pitchSlider.dataToControl = &monitorState->pitch;
        pitchSlider.enabled = true;
        pitchSlider.withFontSize(DEFAULT_FONT_SIZE);
        pitchSlider.draw();
        
        m.prepare<M1Label>({310, 70, 320, 90}).text("PITCH").draw();
        
        if (pitchSlider.changed) {
            double normalisedValue = ( processor->parameters.getParameter(processor->paramPitch)->convertTo0to1(monitorState->pitch) - 0.5 ) / 2;
            processor->parameters.getParameter(processor->paramPitch)->setValueNotifyingHost(normalisedValue);
        }
        
        auto& rollSlider = m.prepare<M1Slider>({   317, 148, 129, 68 })
        //                                            .withLabel("ROLL")
            .hasMovingLabel(true)
            .drawHorizontal(true);
        rollSlider.cursorHide = cursorHide;
        rollSlider.cursorShow = cursorShow;
        rollSlider.rangeFrom = -90.;
        rollSlider.rangeTo = 90.;
        rollSlider.dataToControl = &monitorState->roll;
        rollSlider.enabled = true;
        rollSlider.withFontSize(DEFAULT_FONT_SIZE);
        rollSlider.draw();
        
        if (rollSlider.changed) {
            double normalisedValue = (processor->parameters.getParameter(processor->paramRoll)->convertTo0to1(monitorState->roll) - 0.5 ) / 2;
            processor->parameters.getParameter(processor->paramRoll)->setValueNotifyingHost(normalisedValue);
        }
        
        m.prepare<M1Label>({355, 150, 70, 50}).text("ROLL").draw();
        
        /// CHECKBOXES
        
        float checkboxSlotHeight = 30;
        
        auto& yawActive = m.prepare<M1Checkbox>({ 267, 410 + checkboxSlotHeight * 3,
            100, 30 })
        .controlling(&monitorState->yawActive)
        .withLabel("Y");
        yawActive.enabled = true;
        yawActive.draw();
        
        if (yawActive.changed) {
            processor->parameterChanged(processor->paramYawEnable, monitorState->yawActive);
        }
        
        auto& pitchActive = m.prepare<M1Checkbox>({ 307, 410 + checkboxSlotHeight * 3,
            100, 30 })
        .controlling(&monitorState->pitchActive)
        .withLabel("P");
        pitchActive.enabled = true;
        pitchActive.draw();
        
        if (pitchActive.changed) {
            processor->parameterChanged(processor->paramPitchEnable, monitorState->pitchActive);
        }
        
        auto& rollActive = m.prepare<M1Checkbox>({ 347, 410 + checkboxSlotHeight * 3,
            100, 30 })
        .controlling(&monitorState->rollActive)
        .withLabel("R");
        rollActive.enabled = true;
        rollActive.draw();
        
        if (rollActive.changed) {
            processor->parameterChanged(processor->paramRollEnable, monitorState->rollActive);
        }
>>>>>>> 9ae16342730e98f826c2e5454085ce5de8cc5cff
    }
    
    auto& recenterActive = m.prepare<M1Checkbox>({ 420, 420,
                                                200, 20 })
                                                .controlling(&monitorState->monitor_mode)
                                                .withLabel("RECENTER");
    recenterActive.enabled = true;
    recenterActive.draw();

//    if (recenterActive.changed) {
//        processor->parameterChanged(processor->paramRollEnable, monitorState->rollActive);
//    }
    
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
    
    /// Monitor Settings button
    if (showSettingsMenu) {
        auto& showSettingsButton = m.prepare<M1DropdownButton>({ m.getSize().width()/2 - 40, 440,
            100, 20 })
        .withLabel("SETTINGS")
        .withOutline(false).draw();
        if (showSettingsButton.pressed) {
            showSettingsMenu = false;
            deleteTheSettingsButton();
        }
    } else {
        auto& showSettingsButton2 = m.prepare<M1DropdownButton>({ m.getSize().width()/2 - 40, 225,
            100, 20 })
        .withLabel("SETTINGS")
        .withOutline(false).draw();
        if (showSettingsButton2.pressed) {
            showSettingsMenu = true;
            deleteTheSettingsButton();
        }
    }
    
    // draw Settings button arrow
    // TODO: fix and fill arrow
    if (showSettingsMenu) {
        // draw settings arrow indicator pointing up
        m.enableFill();
        m.setColor(LABEL_TEXT_COLOR);
        MurkaPoint triangleCenter = {m.getSize().width()/2 + 55, m.getSize().height() - 14};
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
        MurkaPoint triangleCenter = {m.getSize().width()/2 + 55, m.getSize().height() - 14};
        std::vector<MurkaPoint3D> triangle;
        triangle.push_back({triangleCenter.x - 5, triangleCenter.y, 0});
        triangle.push_back({triangleCenter.x + 5, triangleCenter.y, 0}); // top middle
        triangle.push_back({triangleCenter.x , triangleCenter.y - 5, 0});
        triangle.push_back({triangleCenter.x - 5, triangleCenter.y, 0});
        m.drawPath(triangle);
    }
    
    /// Monitor label
    m.setColor(200, 255);
    m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, DEFAULT_FONT_SIZE);
#ifdef CUSTOM_CHANNEL_LAYOUT
    auto& monitorLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() - 100, m.getSize().height() - 30, 80, 20));
#else
    int labelYOffset;
    if (!processor->hostType.isProTools() || (processor->hostType.isProTools() && (processor->getMainBusNumInputChannels() > 2 || processor->getMainBusNumOutputChannels() > 2))) {
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
    m.drawImage(m1logo, 20, m.getSize().height() - 30, 161 / 3, 39 / 3);
#else
    m.drawImage(m1logo, 20, m.getSize().height() - labelYOffset, 161 / 3, 39 / 3);
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
