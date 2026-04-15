#pragma once

#include <QString>
#include <QMetaType>
#include "EXMIDIEvent.h"

enum class InputBehavior {
    Knob,
    Button,
    Switch
};
Q_DECLARE_METATYPE(InputBehavior); // So it can be used in Comboboxes directly


inline const QString inputBehaviorToString(const InputBehavior b) {
    switch (b) {
        case InputBehavior::Knob: return "Knob";
        case InputBehavior::Button: return "Button";
        case InputBehavior::Switch: return "Switch";
    }
    return "Unknown";
}


inline InputBehavior inputBehaviorFromString(const QString s) {
    if (s == "Knob") { return InputBehavior::Knob; }
    else if (s == "Button") { return InputBehavior::Button; }
    else if (s == "Switch") { return InputBehavior::Switch; }
    else { return InputBehavior::Knob; }
}

inline std::array<QString, 3> InputBehaviors() {
    return { "Knob", "Button", "Switch" };
}

enum class EXMappedMidiAction {
        TurnKnob1=0,
        TurnKnob2=1,
        TurnKnob3=2,
        TurnKnob4=3,
        TurnKnob5=4,
        TurnKnob6=5,
        TurnKnob7=6,
        TurnKnob8=7,
        SelectPreset1=8,
        SelectPreset2=9,
        SelectPreset3=10,
        SelectPreset4=11,
        SelectPreset5=12,
        SelectPreset6=13,
        SelectPreset7=14,
        SelectPreset8=15,
        None=16
};
Q_DECLARE_METATYPE(EXMappedMidiAction);

inline const std::array<const EXMappedMidiAction, 17>& AllEXMappedMidiActions() {
    static const std::array<const EXMappedMidiAction, 17> actions = {
        EXMappedMidiAction::TurnKnob1,     EXMappedMidiAction::TurnKnob2,
        EXMappedMidiAction::TurnKnob3,     EXMappedMidiAction::TurnKnob4,
        EXMappedMidiAction::TurnKnob5,     EXMappedMidiAction::TurnKnob6,
        EXMappedMidiAction::TurnKnob7,     EXMappedMidiAction::TurnKnob8,
        EXMappedMidiAction::SelectPreset1, EXMappedMidiAction::SelectPreset2,
        EXMappedMidiAction::SelectPreset3, EXMappedMidiAction::SelectPreset4,
        EXMappedMidiAction::SelectPreset5, EXMappedMidiAction::SelectPreset6,
        EXMappedMidiAction::SelectPreset7, EXMappedMidiAction::SelectPreset8,
        EXMappedMidiAction::None

    };
    return actions;
}

inline const std::array<const QString, 17>& EXMappedMidiActionTexts() {
    static const std::array<const QString, 17> actions = {
        "Turn Knob 1", "Turn Knob 2", "Turn Knob 3", "Turn Knob 4",
        "Turn Knob 5", "Turn Knob 6", "Turn Knob 7", "Turn Knob 8",
        "Select Preset 1", "Select Preset 2", "Select Preset 3", "Select Preset 4",
        "Select Preset 5", "Select Preset 6", "Select Preset 7", "Select Preset 8",
        "None"
    };
    return actions;
}

inline QString EXMappedMidiActionToString(EXMappedMidiAction action) {
    const std::array<const QString, 17>& actions = EXMappedMidiActionTexts();

    int index = (int)action;
    if (index<0 || index >16) return "Error: EXMappedMidiAction out of bounds";
    else return actions[index];
}

inline EXMappedMidiAction EXMappedMidiActionFromString(QString actionstring) {
    const std::array<const QString, 17>& actions = EXMappedMidiActionTexts();

    for (int k=0; k<17; ++k)
        if (actions[k] == actionstring) return static_cast<EXMappedMidiAction>(k);

    return EXMappedMidiAction::None;
}

inline bool MidiActionMapsToKnob(EXMappedMidiAction action) {
    return (int)action <8;
}

inline bool MidiActionMapsToPad(EXMappedMidiAction action) {
    int a = (int)action;
    return (8<=a) && (a<16);
}

inline int MidiActionDeviceIndex(EXMappedMidiAction action) {
    if (action == EXMappedMidiAction::None) { return -1; }
    else { return (int)action % 8; }
}

struct MappingEntry {
    public:
        static MappingEntry EmptyMappingEntry() { MappingEntry e; return e; }
        static MappingEntry Default(EXMappedMidiAction action);

        MidiEventType eventType = MidiEventType::Unknown;
        int eventCode = 0;
        InputBehavior inputBehavior = InputBehavior::Knob;
        EXMappedMidiAction mappedAction=EXMappedMidiAction::None;

        // Runtime state
        bool isActive = false;
        bool isResponsive = true;  // For Switch only
        int threshold = 6;

        // Switch hysteresis margin (defaults to 10, can be adjusted later)
        int hysteresis = 10;

        //whether it reacts to an event
        bool matchesEvent(const MidiEvent& incomingType) const;
        bool mapsToPad() const;
        bool mapsToKnob() const;
        int deviceIndex() const;
};




















