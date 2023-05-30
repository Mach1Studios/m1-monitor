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
        //setSize(504, 469);
        
        // Settings rendering
        
        //Timecode rect
        m.setColor(GRID_LINES_1_RGBA);
        m.enableFill();
        m.drawRectangle(265, 327, 215, 27);
        
        //Broadcast rect
        m.enableFill();
        m.drawRectangle(265, 272, 215, 27);
        
        //right side
        m.setColor(DISABLED_PARAM);
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, 11);
        m.prepare<murka::Label>({267, 252, 150, 20}).withAlignment(TEXT_LEFT).text("BROADCAST MIX").draw();
        m.setColor(ENABLED_PARAM);
        m.prepare<murka::Label>({267, 305, 150, 20}).withAlignment(TEXT_LEFT).text("TIME CODE OFFSET").draw();
        m.prepare<murka::Label>({267, 365, 150, 20}).withAlignment(TEXT_LEFT).text("OSC PORT").draw();
        m.prepare<murka::Label>({267, 390, 150, 20}).withAlignment(TEXT_LEFT).text("INPUT").draw();
        
        //left side
        m.setColor(ENABLED_PARAM);
        m.prepare<murka::Label>({17, 252, 150, 20}).withAlignment(TEXT_LEFT).text("MONITOR MODE").draw();
    } else {
        // TODO: fix this
        //setSize(504, 266);
    }
    
    m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, 10);

    /// YPR SLIDERS
    auto& yawRadial = m.prepare<M1Radial>({ 35, 25, 189, 189 }).withLabel("YAW");
    yawRadial.cursorHide = cursorHide;
    yawRadial.cursorShow = cursorShow;
    yawRadial.rangeFrom = 0.;
    yawRadial.rangeTo = 360.;
    yawRadial.dataToControl = &monitorState->yaw;
    yawRadial.enabled = true;
    yawRadial.draw();
    
    if (yawRadial.changed) {
//        processor->parameterChanged(processor->paramYaw, monitorState->yaw);
        double normalisedValue = processor->parameters.getParameter(processor->paramYaw)->convertTo0to1(monitorState->yaw - 180);
        processor->parameters.getParameter(processor->paramYaw)->setValueNotifyingHost(normalisedValue);
    }
    
    auto& pitchSlider = m.prepare<M1Slider>({  327, 26, 108, 108 })
                                            .withLabel("PITCH")
                                            .hasMovingLabel(true)
                                            .drawHorizontal(false);
    pitchSlider.cursorHide = cursorHide;
    pitchSlider.cursorShow = cursorShow;
    pitchSlider.rangeFrom = -90.;
    pitchSlider.rangeTo = 90.;
    pitchSlider.dataToControl = &monitorState->pitch;
    pitchSlider.enabled = true;
    pitchSlider.draw();
    
    if (pitchSlider.changed) {
        double normalisedValue = processor->parameters.getParameter(processor->paramPitch)->convertTo0to1(monitorState->pitch);
        processor->parameters.getParameter(processor->paramPitch)->setValueNotifyingHost(normalisedValue);
    }
        
    auto& rollSlider = m.prepare<M1Slider>({   317, 148, 129, 68 })
                                            .withLabel("ROLL")
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
        double normalisedValue = processor->parameters.getParameter(processor->paramRoll)->convertTo0to1(monitorState->roll);
        processor->parameters.getParameter(processor->paramRoll)->setValueNotifyingHost(normalisedValue);
    }
    
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
    
    /// Monitor Settings button
    auto& showSettingsButton = m.prepare<M1DropdownButton>({ m.getSize().width()/2 - 40, 200,
                                                100, 20 })
                                                .withLabel("SETTINGS      ").draw();
    
    if (showSettingsButton.pressed) {
        showSettingsMenu = !showSettingsMenu;
        if (showSettingsMenu) {
            windowResize(504, 266);
        } else {
            windowResize(504, 200);

        }
    }
    
    // draw Settings button arrow
    // TODO: fix and fill arrow
    if (showSettingsMenu) {
        // draw settings arrow indicator pointing up
        m.enableFill();
        m.setColor(LABEL_TEXT_COLOR);
        std::vector<MurkaPoint3D> triangle;
        triangle.push_back({(m.getSize().width()/2 - 40) + 85, m.getSize().height() - 10, 0});
        triangle.push_back({(m.getSize().width()/2 - 40) + 95, m.getSize().height() - 15, 0}); // top middle
        triangle.push_back({(m.getSize().width()/2 - 40) + 95, m.getSize().height() - 10, 0});
        triangle.push_back({(m.getSize().width()/2 - 40) + 85, m.getSize().height() - 10, 0});
        m.drawPath(triangle);
    } else {
        // draw settings arrow indicator pointing down
        m.enableFill();
        m.setColor(LABEL_TEXT_COLOR);
        std::vector<MurkaPoint3D> triangle;
        triangle.push_back({(m.getSize().width()/2 - 40) + 85, m.getSize().height() - 15, 0});
        triangle.push_back({(m.getSize().width()/2 - 40) + 90, m.getSize().height() - 10, 0}); // bottom middle
        triangle.push_back({(m.getSize().width()/2 - 40) + 95, m.getSize().height() - 15, 0});
        triangle.push_back({(m.getSize().width()/2 - 40) + 85, m.getSize().height() - 15, 0});
        m.drawPath(triangle);
    }
    
    /// Monitor label
    m.setColor(200, 255);
    m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, 10);
    auto& pannerLabel = m.prepare<M1Label>(MurkaShape(m.getSize().width() - 100, m.getSize().height() - 30, 80, 20));
    pannerLabel.label = "MONITOR";
    pannerLabel.alignment = TEXT_CENTER;
    pannerLabel.enabled = false;
    pannerLabel.highlighted = false;
    pannerLabel.draw();
    
    m.setColor(200, 255);
    m1logo.loadFromRawData(BinaryData::mach1logo_png, BinaryData::mach1logo_pngSize);
    m.drawImage(m1logo, 20, m.getSize().height() - 30, 161 / 3, 39 / 3);

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
