#include "MonitorUIBaseComponent.h"

using namespace murka;

//==============================================================================
MonitorUIBaseComponent::MonitorUIBaseComponent(M1MonitorAudioProcessor* processor_)
{
	// Make sure you set the size of the component after
    // you add any child components.
	setSize (getWidth(), getHeight());

	processor = processor_;
    monitorState = &processor->monitorState;
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

void MonitorUIBaseComponent::render()
{
    // This clears the context with our background.
    //juce::OpenGLHelpers::clear(juce::Colour(255.0, 198.0, 30.0));

    currentMousePosition = m.currentContext.mousePosition * 0.7;
    
    float scale = (float)openGLContext.getRenderingScale() * 0.7; // (Desktop::getInstance().getMainMouseSource().getScreenPosition().x / 300.0); //  0.7;

    if (scale != m.getScreenScale()) {
        m.setScreenScale(scale);
        m.updateFontsTextures(&m);
        m.clearFontsTextures();
    }

	m.begin();
    m.setFont("Proxima Nova Reg.ttf", 10);

	m.setColor(BACKGROUND_GREY);
	m.clear();

    /// YPR SLIDERS
    auto& yawRadial = m.draw<M1Radial>({ 32, 96, 150, 24 }).withLabel("YAW");
    yawRadial.cursorHide = cursorHide;
    yawRadial.cursorShow = cursorShow;
    yawRadial.rangeFrom = 0.;
    yawRadial.rangeTo = 360.;
    yawRadial.dataToControl = &monitorState->yaw;
    yawRadial.commit();
    
    if (yawRadial.changed) {
        processor->parameterChanged(processor->paramYaw, monitorState->yaw);
    }
    
    auto& pitchSlider = m.draw<M1Slider>({  208, 8, 41, 224 })
                                            .withLabel("PITCH")
                                            .hasMovingLabel(true)
                                            .drawHorizontal(false);
    pitchSlider.cursorHide = cursorHide;
    pitchSlider.cursorShow = cursorShow;
    pitchSlider.rangeFrom = 90.;
    pitchSlider.rangeTo = -90.;
    pitchSlider.dataToControl = &monitorState->pitch;
    pitchSlider.commit();
    
    if (pitchSlider.changed) {
        processor->parameterChanged(processor->paramPitch, monitorState->pitch);
    }
        
    auto& rollSlider = m.draw<M1Slider>({   8, 232, 200, 30 })
                                            .withLabel("ROLL")
                                            .hasMovingLabel(true)
                                            .drawHorizontal(true);
    rollSlider.cursorHide = cursorHide;
    rollSlider.cursorShow = cursorShow;
    rollSlider.rangeFrom = 90.;
    rollSlider.rangeTo = -90.;
    rollSlider.dataToControl = &monitorState->roll;
    rollSlider.commit();
    
    if (rollSlider.changed) {
        processor->parameterChanged(processor->paramRoll, monitorState->roll);
    }
    
	/// CHECKBOXES
	float checkboxSlotHeight = 28;
    
//    auto& autoOrbitCheckbox = m.draw<M1Checkbox>({ 557, 475 + checkboxSlotHeight * 3,
//                                                200, 20 })
//                                                .controlling(&pannerState->autoOrbit)
//                                                .withLabel("AUTO ORBIT");
//    autoOrbitCheckbox.enabled = (pannerState->m1Encode->getInputMode() != Mach1EncodeInputModeMono);
//    autoOrbitCheckbox.commit();
//
//    if (autoOrbitCheckbox.changed) {
//        processor->parameterChanged(processor->paramAutoOrbit, pannerState->autoOrbit);
//    }
    
    /// Monitor label
    m.setColor(200, 255);
    m.setFont("Proxima Nova Reg.ttf", 10);
    auto& pannerLabel = m.draw<M1Label>(MurkaShape(m.getSize().width() - 100, m.getSize().height() - 30, 80, 20));
    pannerLabel.label = "MONITOR";
    pannerLabel.alignment = TEXT_CENTER;
    pannerLabel.enabled = false;
    pannerLabel.highlighted = false;
    pannerLabel.commit();
    
    m.setColor(200, 255);
    m1logo.loadFromRawData(BinaryData::mach1logo_png, BinaryData::mach1logo_pngSize);
    m.drawImage(m1logo, 20, m.getSize().height() - 30, 161 / 3, 39 / 3);

	m.end();
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
