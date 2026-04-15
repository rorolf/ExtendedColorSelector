


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

