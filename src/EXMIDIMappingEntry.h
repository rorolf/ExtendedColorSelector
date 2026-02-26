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

struct MappingEntry {
    MidiEventType eventType;
    int code;
    InputBehavior behavior;
    QString mappedAction;

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




















