#pragma once

#include <QString>

enum class MidiEventType {
    NoteOn,
    NoteOff,
    ControlChange,
    Unknown
};

struct MidiEvent {
    MidiEventType type;
    int channel;
    int code;     // note number or CC number
    int value;    // velocity or controller value

    QString toString() const;
    bool isValid() const;
    static MidiEvent fromRtMidiMessage(const QByteArray& data);
};

inline QString midiEventTypeToString(MidiEventType t) {
    switch (t) {
        case MidiEventType::NoteOn: return "NoteOn";
        case MidiEventType::NoteOff: return "NoteOff";
        case MidiEventType::ControlChange: return "ControlChange";
        case MidiEventType::Unknown: return "Unknown";
    }
    return "Unknown";
}


inline const MidiEventType midiEventTypeFromString(const QString s) {
    if (s == "NoteOn") { return MidiEventType::NoteOn; }
    else if (s == "NoteOff") { return MidiEventType::NoteOff; }
    else if (s == "ControlChange") { return MidiEventType::ControlChange; }
    //else if (s == "Unknown") { return MidiEventType::Unknown; }
    else { return MidiEventType::Unknown; }
}
