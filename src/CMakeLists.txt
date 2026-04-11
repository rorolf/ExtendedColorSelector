set(
    extendedcolorselector_SOURCES
    EXChannelPlane.cpp
    EXChannelSlider.cpp
    EXColorModel.cpp
    EXColorModelSwitchers.cpp
    EXColorPatchPopup.cpp
    EXColorSelectorDock.cpp
    EXColorSelectorPlugin.cpp
    EXColorState.cpp
    EXEditable.cpp
    EXGamutClipping.cpp
    EXKoColorConverter.cpp
    EXPortableColorSelector.cpp
    EXSettings.cpp
    EXSettingsDialog.cpp
    EXSettingsState.cpp
    EXShape.cpp
    EXUtils.cpp
    OptionalColorPicker.cpp

    RtMidi.cpp
    EXMIDIEvent.cpp
    EXMIDIMappingEntry.cpp
    EXMIDIMapper_PresetControl.cpp
    EXMIDIListener.cpp
    EXMIDIDemultiplexer.h
    EXMIDILogPanelWidget.cpp
    EXMIDIMappingTableWidget.cpp
    EXMIDIPanelWidget.cpp

    EXColorMixState.cpp
    EXColorPresetStore.cpp
    EXColorMixerDock.cpp
    EXColorPatchWidget.cpp
    EXActionbus.cpp

)


set(CMAKE_BUILD_TYPE Debug)
set(RTMIDI_API_ALSA ON)
set(RTMIDI_API_JACK OFF)

qt5_add_resources(extendedcolorselector_RESOURCES resources.qrc)





kis_add_library(extendedcolorselector MODULE ${extendedcolorselector_SOURCES} ${extendedcolorselector_RESOURCES})

# may be valid for docker container, if configured like that
#set(RTMIDI_LIB_PATH /home/appimage/persistent/deps/librtmidi.so)
#target_link_libraries(extendedcolorselector PRIVATE ${RTMIDI_LIB_PATH})
list(APPEND CMAKE_PREFIX_PATH "/home/appimage/persistent/deps/")
#find_package(RtMidi REQUIRED)
#target_link_libraries(extendedcolorselector PRIVATE RtMidi::rtmidi)
#add_subdirectory(libremidi)
#target_compile_definitions(extendedcolorselector PRIVATE NOMINMAX)
#target_link_libraries(extendedcolorselector PRIVATE libremidi)
target_link_libraries(extendedcolorselector PRIVATE portmidi)
# target_link_libraries(extendedcolorselector PRIVATE
#     "${CMAKE_SOURCE_DIR}/external/PortMidi/build/libportmidi.a"
# )
target_include_directories(extendedcolorselector PRIVATE
    "${CMAKE_SOURCE_DIR}/external/PortMidi/pm_common"
    "${CMAKE_SOURCE_DIR}/external/PortMidi/portmidi"
)

target_link_libraries(extendedcolorselector PRIVATE rtmidi)
target_include_directories(extendedcolorselector PRIVATE
    "${CMAKE_SOURCE_DIR}/external/rtmidi"
)

# Platform-specific linking
if(WIN32)
    target_link_libraries(extendedcolorselector PRIVATE winmm)
elseif(APPLE)
    find_library(COREMIDI_LIBRARY CoreMIDI)
    target_link_libraries(extendedcolorselector PRIVATE ${COREMIDI_LIBRARY})
elseif(UNIX AND NOT APPLE)
    find_library(ASOUND_LIBRARY asound)
    target_link_libraries(extendedcolorselector PRIVATE ${ASOUND_LIBRARY} pthread)
endif()


generate_export_header(extendedcolorselector BASE_NAME extendedcolorselector)

#target_include_directories(extendedcolorselector PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/libremidi/include)
#set_target_properties(extendedcolorselector PROPERTIES CXX_STANDARD 23)

# Link platform-specific libraries if needed
# if(UNIX AND NOT APPLE)
#     # For ALSA on Linux
#     find_package(ALSA REQUIRED)
#     target_link_libraries(extendedcolorselector PRIVATE asound)
# elseif(APPLE)
#     # For CoreAudio on macOS
#     find_library(COREAUDIO_LIBRARY CoreAudio)
#     find_library(COREMIDI_LIBRARY CoreMIDI)
#     target_link_libraries(extendedcolorselector PRIVATE ${COREAUDIO_LIBRARY} ${COREMIDI_LIBRARY})
# elseif(WIN32)
#     # For WinMM on Windows
#     target_link_libraries(extendedcolorselector PRIVATE winmm)
# endif()



# target_compile_features(extendedcolorselector PRIVATE cxx_std_20)
target_link_libraries(extendedcolorselector PRIVATE kritaui)
target_include_directories(extendedcolorselector PRIVATE
    ${CMAKE_SOURCE_DIR}/libs
)

set_target_properties(extendedcolorselector PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN"
)
