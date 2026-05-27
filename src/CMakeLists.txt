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

    EXMIDIEvent.cpp
    EXMIDIMappingEntry.cpp
    EXMIDIMapper_PresetControl.cpp
    EXMIDIListener.cpp
    EXMIDIDemultiplexer.h
    EXMIDILogPanelWidget.cpp
    EXMIDIMappingTableWidget.cpp
    EXMIDIPanelWidget.cpp

    EXGradient.h
    EXGradientWidget.cpp
    EXColorMixState.cpp
    EXColorPresetStore.cpp
    EXColorMixerDock.cpp
    EXColorPatchWidget.cpp
    EXActionbus.cpp

)


qt5_add_resources(extendedcolorselector_RESOURCES resources.qrc)





kis_add_library(extendedcolorselector MODULE ${extendedcolorselector_SOURCES} ${extendedcolorselector_RESOURCES})

list(APPEND CMAKE_PREFIX_PATH "/home/appimage/persistent/deps/")

target_link_libraries(extendedcolorselector PRIVATE portmidi)
target_include_directories(extendedcolorselector PRIVATE
    "${CMAKE_SOURCE_DIR}/external/PortMidi/pm_common"
    "${CMAKE_SOURCE_DIR}/external/PortMidi/portmidi"
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



target_link_libraries(extendedcolorselector PRIVATE kritaui)
target_include_directories(extendedcolorselector PRIVATE
    ${CMAKE_SOURCE_DIR}/libs
)

set_target_properties(extendedcolorselector PROPERTIES
    BUILD_WITH_INSTALL_RPATH TRUE
    INSTALL_RPATH "$ORIGIN"
)
