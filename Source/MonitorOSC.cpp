#include "MonitorOSC.h"

bool MonitorOSC::init(int helperPort) {
    // check port
    juce::DatagramSocket socket(false);
    socket.setEnablePortReuse(false);
    this->helperPort = helperPort;
    
    // find available port
    for (port = 10201; port < 10300; port++) {
        if (socket.bindToPort(port)) {
            socket.shutdown();
            juce::OSCReceiver::connect(port);
            break; // stops the incrementing on the first available port
        }
    }
    
    if (port > 10200) {
        return true;
    }
}

// finds the server port via the settings json file
bool MonitorOSC::initFromSettings(std::string jsonSettingsFilePath) {
    juce::File settingsFile = juce::File(jsonSettingsFilePath);
    if (!settingsFile.exists()) {
        juce::AlertWindow::showMessageBoxAsync(
            juce::AlertWindow::NoIcon,
            "Warning",
            "Settings file doesn't exist",
            "",
            nullptr,
            juce::ModalCallbackFunction::create(([&](int result) {
                //juce::JUCEApplicationBase::quit();
            }))
        );
        return false;
    }
    else {
        juce::var mainVar = juce::JSON::parse(juce::File(jsonSettingsFilePath));
        int helperPort = mainVar["helperPort"];

        if (!init(helperPort)) {
            juce::AlertWindow::showMessageBoxAsync(
                juce::AlertWindow::WarningIcon,
                "Warning",
                "Conflict is happening and you need to choose a new port",
                "",
                nullptr,
                juce::ModalCallbackFunction::create(([&](int result) {
                   // juce::JUCEApplicationBase::quit();
                }))
            );
            return false;
        }
    }
    return true;
}

MonitorOSC::MonitorOSC()
{
	isConnected = false;
    
    // We will assume the folders are properly created during the installation step
    juce::File settingsFile;
    // Using common support files installation location
    juce::File m1SupportDirectory = juce::File::getSpecialLocation(juce::File::commonApplicationDataDirectory);

    if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::MacOSX) != 0) {
        // test for any mac OS
        settingsFile = m1SupportDirectory.getChildFile("Application Support").getChildFile("Mach1");
    } else if ((juce::SystemStats::getOperatingSystemType() & juce::SystemStats::Windows) != 0) {
        // test for any windows OS
        settingsFile = m1SupportDirectory.getChildFile("Mach1");
    } else {
        settingsFile = m1SupportDirectory.getChildFile("Mach1");
    }
    settingsFile = settingsFile.getChildFile("settings.json");
    DBG("Opening settings file: " + settingsFile.getFullPathName().quoted());
    
    initFromSettings(settingsFile.getFullPathName().toStdString());
    juce::OSCReceiver::addListener(this);
}

void MonitorOSC::oscMessageReceived(const juce::OSCMessage& msg)
{
    if (messageReceived != nullptr) {
        messageReceived(msg);
    }
    lastMessageTime = juce::Time::getMillisecondCounter();
}

void MonitorOSC::update()
{
	if (!isConnected && helperPort > 0) {
		if (juce::OSCSender::connect("127.0.0.1", helperPort)) {
            juce::OSCMessage msg = juce::OSCMessage(juce::OSCAddressPattern("/m1-addClient"));
			msg.addInt32(port);
            msg.addString("monitor");
			isConnected = juce::OSCSender::send(msg);
            DBG("[OSC] Monitor registered as: "+std::to_string(port));
		}
	}
    
    if (isConnected) {
        juce::uint32 currentTime = juce::Time::getMillisecondCounter();
        if ((currentTime - lastMessageTime) > 10000) { // 10000 milliseconds = 10 seconds
            isConnected = false;
        }
    }
}

void MonitorOSC::AddListener(std::function<void(juce::OSCMessage msg)> messageReceived)
{
	this->messageReceived = messageReceived;
}

MonitorOSC::~MonitorOSC()
{
    if (isConnected && helperPort > 0) {
        if (juce::OSCSender::connect("127.0.0.1", helperPort)) {
            juce::OSCMessage msg = juce::OSCMessage(juce::OSCAddressPattern("/m1-removeClient"));
            msg.addInt32(port);
            msg.addString("monitor");
            isConnected = juce::OSCSender::send(msg);
            DBG("[OSC] Registered: "+std::to_string(port));
        }
    }

    juce::OSCSender::disconnect();
    juce::OSCReceiver::disconnect();
    
    // reset the port
    port = 0;
}

bool MonitorOSC::Send(const juce::OSCMessage& msg)
{
	return (isConnected && juce::OSCSender::send(msg));
}

bool MonitorOSC::IsConnected()
{
	return isConnected;
}

bool MonitorOSC::IsActiveMonitor()
{
    return isActiveMonitor;
}

void MonitorOSC::setAsActiveMonitor(bool is_active)
{
    isActiveMonitor = is_active;
}

bool MonitorOSC::sendMonitoringMode(int mode)
{
    if (port > 0) {
        juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setMonitoringMode"));
        m.addInt32(mode);  // int of monitor mode
        isConnected = juce::OSCSender::send(m); // check to update isConnected for error catching
        return true;
    }
    return false;
}

bool MonitorOSC::sendMasterYPR(float yaw, float pitch, float roll)
{
    if (port > 0) {
        juce::OSCMessage m = juce::OSCMessage(juce::OSCAddressPattern("/setMasterYPR"));
        m.addFloat32(yaw);   // expected degrees -180->180
        m.addFloat32(pitch); // expected degrees -90->90
        m.addFloat32(roll);  // expected degrees -90->90
        isConnected = juce::OSCSender::send(m); // check to update isConnected for error catching
        return true;
    }
    return false;
}
