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


inline QString inputBehaviorToString(InputBehavior b) {
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


enum class EXMappedMidiAction {
    SelectPreset1=0,
    SelectPreset2=1,
    SelectPreset3=2,
    SelectPreset4=3,
    SelectPreset5=4,
    SelectPreset6=5,
    SelectPreset7=6,
    SelectPreset8=7,
    TurnKnob1=8,
    TurnKnob2=9,
    TurnKnob3=10,
    TurnKnob4=11,
    TurnKnob5=12,
    TurnKnob6=13,
    TurnKnob7=14,
    TurnKnob8=15,
    None=16
};
Q_DECLARE_METATYPE(EXMappedMidiAction);

inline QString EXMappedMidiActionToString(EXMappedMidiAction action) {
    std::array<QString, 17> actions = {
            "Select Preset 1", "Select Preset 2", "Select Preset 3", "Select Preset 4",
            "Select Preset 5", "Select Preset 6", "Select Preset 7", "Select Preset 8",
            "Turn Knob 1", "Turn Knob 2", "Turn Knob 3", "Turn Knob 4",
            "Turn Knob 5", "Turn Knob 6", "Turn Knob 7", "Turn Knob 8",
            "None"
        };

    int index = (int)action;
    if (index<0 || index >16) return "Error: EXMappedMidiAction out of bounds";
    else return actions[index];
}

inline EXMappedMidiAction EXMappedMidiActionFromString(QString actionstring) {
    std::array<QString, 17> actions = {
            "Select Preset 1", "Select Preset 2", "Select Preset 3", "Select Preset 4",
            "Select Preset 5", "Select Preset 6", "Select Preset 7", "Select Preset 8",
            "Turn Knob 1", "Turn Knob 2", "Turn Knob 3", "Turn Knob 4",
            "Turn Knob 5", "Turn Knob 6", "Turn Knob 7", "Turn Knob 8",
            "None"
        };

    for (int k=0; k<16; ++k)
        if (actions[k] == actionstring) return static_cast<EXMappedMidiAction>(k);

    return EXMappedMidiAction::None;
}



struct MappingEntry {
    public:
        MidiEventType eventType;
        int eventCode;
        InputBehavior inputBehavior;
        EXMappedMidiAction mappedAction;

        // Runtime state
        bool isActive = false;
        bool isResponsive = true;  // For Switch only
        int threshold = 6;

        // Switch hysteresis margin (defaults to 10, can be adjusted later)
        int hysteresis = 10;

        //whether it reacts to an event
        bool matchesEvent(const MidiEvent& incomingType);
        // Logic handler
        bool processInput(int value, QString& resultText);
};




















