#include "EXMIDIMappingEntry.h"

bool MappingEntry::matchesEvent(const MidiEvent& incomingEvent) {
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


// returns true when signal is active
bool MappingEntry::processInput(int value, QString& resultText)
{
    resultText.clear();

    switch (inputBehavior) {
        case InputBehavior::Knob:
            resultText = QString("Value: %1").arg(value);
            return true;

        case InputBehavior::Button: {
            bool nowActive = value >= threshold;
            if (nowActive != isActive) {
                isActive = nowActive;
                resultText = QString("Button %1").arg(isActive ? "ON" : "OFF");
                return true;
            } else {
                // resultText = QString("Button: No reaction");
                // return true;
            }
            break;
        }

        case InputBehavior::Switch: {
            // Adjusted: Cap hysteresis to not exceed threshold
            bool above = value >= threshold;
            bool significantlyBelow = value <= qMax(threshold - hysteresis, 0);

            if (isResponsive && above) {
                isActive = !isActive;         // Flip ON <-> OFF
                isResponsive = false;
                resultText = QString("Switch toggled %1").arg(isActive ? "ON" : "OFF");
                return true;
            } else if (!isResponsive && significantlyBelow) {
                isResponsive = true;
                // resultText = QString("Switch: Now responsive again");
                // return true;
            } else {
                // resultText = QString("Switch: No reaction");
                // return true;
            }
            break;
        }
    }

    return false;
}
