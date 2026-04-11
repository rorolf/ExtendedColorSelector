#pragma once


#include <QMetaType>
#include <QString>

#include <portmidi.h>

enum class MidiEventType {
    NoteOn,
    NoteOff,
    ControlChange,
    Unknown
};
Q_DECLARE_METATYPE(MidiEventType);

const int NOTEOFF = 0x80;
const int NOTEON = 0x90;
const int CTRLCHANGE = 0xB0;

inline static const std::array<const MidiEventType, 4> AllMidiEventTypes() {
    static const std::array<const MidiEventType, 4> types = {
        MidiEventType::NoteOn, MidiEventType::NoteOff,
        MidiEventType::ControlChange, MidiEventType::Unknown
    };
    return types;
}

inline static const std::array<const QString, 4> AllMidiEventTypeTexts() {
    static const std::array<const QString, 4> typeTexts = {
        "NoteOn", "NoteOff",
        "ControlChange", "Unknown"
    };
    return typeTexts;
}

inline QString midiEventTypeToString(MidiEventType t) {
    switch (t) {
        case MidiEventType::NoteOn: return "NoteOn";
        case MidiEventType::NoteOff: return "NoteOff";
        case MidiEventType::ControlChange: return "ControlChange";
        case MidiEventType::Unknown: return "Unknown";
    }
    return "Unknown";
}


inline MidiEventType midiEventTypeFromString(const QString s) {
    if (s == "NoteOn") { return MidiEventType::NoteOn; }
    else if (s == "NoteOff") { return MidiEventType::NoteOff; }
    else if (s == "ControlChange") { return MidiEventType::ControlChange; }
    //else if (s == "Unknown") { return MidiEventType::Unknown; }
    else { return MidiEventType::Unknown; }
}



struct MidiEvent {
    MidiEventType type;
    int channel;
    int code;     // note number or CC number
    int value;    // velocity or controller value

    QString toString() const;
    bool isValid() const;
    static MidiEvent fromRtMidiMessage(const QByteArray& data);
    static MidiEvent fromPortMidiMessage(const PmEvent& data);
};
Q_DECLARE_METATYPE(MidiEvent)
