# Avoid warning about DOWNLOAD_EXTRACT_TIMESTAMP in CMake 3.24:
if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
	cmake_policy(SET CMP0135 NEW)
endif()

include(FetchContent)

# Fetch the latest pre-built libs
FetchContent_Declare(
  m1-sdk
  URL      https://github.com/Mach1Studios/m1-sdk/releases/download/2b1f29b/mach1spatial-libs.zip
)

FetchContent_GetProperties(m1-sdk)

if (NOT m1-sdk_POPULATED)
    FetchContent_Populate(m1-sdk)

    # Perform arbitrary actions on the m1-sdk project
    # Avoid `add_subdirectory()` until a CMakeFile.txt is added to this directory
    set(MACH1SPATIAL_LIBS_PATH "${m1-sdk_SOURCE_DIR}")
endif()

if(APPLE)
    set(MACH1SPATIAL_LIBS_UNIX_PATH ${MACH1SPATIAL_LIBS_PATH}/xcode)
else()
    set(MACH1SPATIAL_LIBS_UNIX_PATH ${MACH1SPATIAL_LIBS_PATH}/linux)
endif()

# link libraries
find_library(MACH1DECODE_LIBRARY 
             NAMES Mach1DecodeCAPI libMach1DecodeCAPI libMach1DecodeCAPI.a libMach1DecodeCAPI.so libMach1DecodeCAPI.lib
             PATHS ${MACH1SPATIAL_LIBS_UNIX_PATH}/lib ${MACH1SPATIAL_LIBS_PATH}/windows-x86 ${MACH1SPATIAL_LIBS_PATH}/windows-x86_64
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_compile_definitions(M1_STATIC)
    message(STATUS "Adding windows OS flags")
endif()

# include headers
set(MACH1SPATIAL_INCLUDES ${MACH1SPATIAL_LIBS_PATH}/linux/include ${MACH1SPATIAL_LIBS_PATH}/linux/include/M1DSP ${MACH1SPATIAL_LIBS_PATH}/xcode/include ${MACH1SPATIAL_LIBS_PATH}/xcode/include/M1DSP ${MACH1SPATIAL_LIBS_PATH}/windows-x86/include ${MACH1SPATIAL_LIBS_PATH}/windows-x86/include/M1DSP)
include_directories(${MACH1SPATIAL_INCLUDES})
