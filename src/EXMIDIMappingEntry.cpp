


#include "EXMIDIMappingEntry.h"
#include <cmath>


MappingEntry MappingEntry::Default(EXMappedMidiAction action) {
    MappingEntry entry;
    if (action == EXMappedMidiAction::None) return entry;

    entry.mappedAction = action;
    if (entry.mapsToKnob()) {
        entry.eventType = MidiEventType::ControlChange;
        entry.inputBehavior = InputBehavior::Knob;
    }
    else if (entry.mapsToPad()) {
        entry.eventType = MidiEventType::NoteOn;
        entry.inputBehavior = InputBehavior::Button;
    }

    return entry;
}

bool MappingEntry::matchesEvent(const MidiEvent& incomingEvent) const {
    if (this->eventCode != incomingEvent.code) {
        return false;
    }

    if (incomingEvent.type == MidiEventType::Unknown) {
        return false;
    }

    if (this->inputBehavior == InputBehavior::Button || this->inputBehavior == InputBehavior::Switch) {
        bool isThisNote = (this->eventType == MidiEventType::NoteOn || this->eventType == MidiEventType::NoteOff);
        bool isIncomingNote = (incomingEvent.type == MidiEventType::NoteOn || incomingEvent.type == MidiEventType::NoteOff);

        bool bothAreNotes = isThisNote && isIncomingNote;
        bool bothAreCC = this->eventType == MidiEventType::ControlChange &&
                         incomingEvent.type == MidiEventType::ControlChange;

        return bothAreNotes || bothAreCC;
    }

    // For Knob (continuous) behavior, type must match exactly
    return this->eventType == incomingEvent.type;
}


bool MappingEntry::mapsToPad() const {
    return MidiActionMapsToPad(this->mappedAction);
}
bool MappingEntry::mapsToKnob() const {
    return MidiActionMapsToKnob(this->mappedAction);
}
int MappingEntry::deviceIndex() const {
    return MidiActionDeviceIndex(this->mappedAction);
}


// // Returns positive values iff an event fired
// // Otherwise, returns the negative value
// // use std::signbit to distinguish +0 from -0
// // might not work on some system-compiler-combinations
// int MappingEntry::mapInput(int value)
// {
//     switch (inputBehavior) {
//         case InputBehavior::Knob:
//             return std::copysign(value, 1); // value must be >1
//
//         case InputBehavior::Button: {
//             bool nowActive = value >= threshold;
//             if (nowActive != isActive) {
//                 isActive = nowActive;
//                 return std::copysign((int)isActive, 1);  // 0 or 1
//             } else {
//                 return std::copysign((int)isActive, -1); // -0 or -1
//             }
//             break;
//         }
//
//         case InputBehavior::Switch: {
//             // Adjusted: Cap hysteresis to not exceed threshold
//             bool above = value >= threshold;
//             bool significantlyBelow = value <= qMax(threshold - hysteresis, 0);
//
//             if (isResponsive && above) {
//                 isActive = !isActive;         // Flip ON <-> OFF
//                 isResponsive = false;
//                 return std::copysign(isActive,1);             // 0 or 1
//             } else if (!isResponsive && significantlyBelow) {
//                 isResponsive = true;
//                 return std::copysign(isActive,-1);            // -0 or -1
//             } else {
//                 return std::copysign(isActive,-1);            // -0 or -1
//             }
//             break;
//         }
//     }
//
//     return false;
// }
//
// // returns true when signal is active
// bool MappingEntry::debugProcessInput(int value, QString& resultText)
// {
//     resultText.clear();
//
//     switch (inputBehavior) {
//         case InputBehavior::Knob:
//             resultText = QString("Value: %1").arg(value);
//             return true;
//
//         case InputBehavior::Button: {
//             bool nowActive = value >= threshold;
//             if (nowActive != isActive) {
//                 isActive = nowActive;
//                 resultText = QString("Button %1").arg(isActive ? "ON" : "OFF");
//                 return true;
//             } else {
//                 // resultText = QString("Button: No reaction");
//                 // return true;
//             }
//             break;
//         }
//
//         case InputBehavior::Switch: {
//             // Adjusted: Cap hysteresis to not exceed threshold
//             bool above = value >= threshold;
//             bool significantlyBelow = value <= qMax(threshold - hysteresis, 0);
//
//             if (isResponsive && above) {
//                 isActive = !isActive;         // Flip ON <-> OFF
//                 isResponsive = false;
//                 resultText = QString("Switch toggled %1").arg(isActive ? "ON" : "OFF");
//                 return true;
//             } else if (!isResponsive && significantlyBelow) {
//                 isResponsive = true;
//                 // resultText = QString("Switch: Now responsive again");
//                 // return true;
//             } else {
//                 // resultText = QString("Switch: No reaction");
//                 // return true;
//             }
//             break;
//         }
//     }
//
//     return false;
// }
