target_sources(${PLUGIN_NAME} PRIVATE    Config.h
                                    AlertData.h
                                    TypesForDataExchange.h
                                    PluginEditor.cpp
                                    PluginEditor.h
                                    PluginProcessor.cpp
                                    PluginProcessor.h
                                    M1SystemHelperManager.cpp
                                    M1SystemHelperManager.h
                                    M1MemoryShare.h
                                    M1MemoryShare.cpp
                                    M1MemoryShareTypes.h
                                    MixBusReader.h
                                    Utility/SharedMemoryPaths.h
                                    Utility/SharedMemoryPaths.cpp
                                    MonitorOSC.h
                                    MonitorOSC.cpp
                                    M1Analytics.h
                                    M1Analytics.cpp
                                    UI/M1Checkbox.h
                                    UI/M1DropdownButton.h
                                    UI/M1DropdownMenu.h
                                    UI/M1Radial.h
                                    UI/M1Slider.h
                                    UI/M1AlertComponent.h
                                    UI/MonitorUIBaseComponent.h
                                    UI/MonitorUIBaseComponent.cpp)

if(APPLE)
    target_sources(${PLUGIN_NAME} PRIVATE Utility/SharedMemoryPaths.mm)
endif(APPLE)
