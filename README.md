# m1-monitor
GUI and plugin concept for Mach1Decode API

#### AAX requires AAX SDK 2.5.1 or higher

## Compiler Options

**It is recommended to compile production plugins via CMake style, all example commands will be supplied at the bottom of this section**

**WARNING: When changing the compiler options make sure to clean the build folder**

### `CUSTOM_CHANNEL_LAYOUT`

##### CMake
- Add as a preprocess definition via `-DCUSTOM_CHANNEL_LAYOUT` and also define the `INPUTS`

Example:
`-DCUSTOM_CHANNEL_LAYOUT=1 -DINPUTS=8`

### Examples

- MacOS setup M1-Monitor
```
cmake -Bbuild -G "Xcode" -DBUILD_VST3=ON -DBUILD_AAX=ON -DAAX_PATH="/Users/[USERNAME]/SDKs/aax-sdk-2-4-1" -DBUILD_AU=ON -DBUILD_VST=ON -DVST2_PATH="/Users/[USERNAME]/SDKs/VST_SDK_vst2/VST2_SDK" -DBUILD_AUV3=ON
```

- MacOS setup & compile M1-Monitor with specific I/O:
```
cmake -Bbuild_i8 -G "Xcode" -DBUILD_WITH_CUSTOM_CHANNEL_LAYOUT=1 -DINPUTS=8 -DOUTPUTS=2 -DBUILD_VST2=1 -DVST2_PATH="/Users/[USERNAME]/SDKs/VST_SDK_vst2/VST2_SDK"
cmake --build build_i8 --config Release
```
