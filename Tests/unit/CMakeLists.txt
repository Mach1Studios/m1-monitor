add_executable(m1-monitor-mode-tests MonitorHostModePolicyTests.cpp)

target_include_directories(m1-monitor-mode-tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../Source)

target_compile_features(m1-monitor-mode-tests PRIVATE cxx_std_17)

add_test(NAME m1-monitor-mode COMMAND m1-monitor-mode-tests)
