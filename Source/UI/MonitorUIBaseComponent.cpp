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
        m.setFontFromRawData(PLUGIN_FONT, BINARYDATA_FONT, BINARYDATA_FONT_SIZE, 11);
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
    } else {
        setShouldResizeTo(MurkaPoint(504, 180));
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
        MurkaPoint triangleCenter = {m.getSize().width()/2 + 55, m.getSize().height() - 12};
        std::vector<MurkaPoint3D> triangle;
        triangle.push_back({triangleCenter.x - 10, triangleCenter.y, 0});
        triangle.push_back({triangleCenter.x + 10, triangleCenter.y, 0}); // top middle
        triangle.push_back({triangleCenter.x , triangleCenter.y - 10, 0});
        triangle.push_back({triangleCenter.x - 10, triangleCenter.y, 0});
        m.drawPath(triangle);
    } else {
        // draw settings arrow indicator pointing down
        m.enableFill();
        m.setColor(LABEL_TEXT_COLOR);
        MurkaPoint triangleCenter = {m.getSize().width()/2 + 55, m.getSize().height() - 12};
        std::vector<MurkaPoint3D> triangle;
        triangle.push_back({triangleCenter.x - 10, triangleCenter.y, 0});
        triangle.push_back({triangleCenter.x + 10, triangleCenter.y, 0}); // top middle
        triangle.push_back({triangleCenter.x , triangleCenter.y - 10, 0});
        triangle.push_back({triangleCenter.x - 10, triangleCenter.y, 0});
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
