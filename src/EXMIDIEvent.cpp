#include "EXMIDIEvent.h"

QString MidiEvent::toString() const {
    QString typeStr;
    switch (type) {
        case MidiEventType::NoteOn: typeStr = "Note On"; break;
        case MidiEventType::NoteOff: typeStr = "Note Off"; break;
        case MidiEventType::ControlChange: typeStr = "CC"; break;
        default: typeStr = "Unknown"; break;
    }

    return QString("%1 Ch%2 Code:%3 Val:%4")
        .arg(typeStr)
        .arg(channel + 1)
        .arg(code)
        .arg(value);
}

MidiEvent MidiEvent::fromRtMidiMessage(const QByteArray& data) {
    MidiEvent evt;
    if (data.size() < 3) {
        evt.type = MidiEventType::Unknown;
        evt.channel = 0;
        evt.code = 0;
        evt.value = 0;
        return evt;
    }

    unsigned char status = static_cast<unsigned char>(data[0]);
    unsigned char data1 = static_cast<unsigned char>(data[1]);
    unsigned char data2 = static_cast<unsigned char>(data[2]);

    int typeCode = status & 0xF0;
    evt.channel = (status & 0x0F) + 1;

    switch (typeCode) {
        case NOTEOFF:
            evt.type = MidiEventType::NoteOff;
            break;
        case NOTEON:
            evt.type = MidiEventType::NoteOn;
            break;
        case CTRLCHANGE:
            evt.type = MidiEventType::ControlChange;
            break;
        default:
            evt.type = MidiEventType::Unknown;
            break;
    }

    evt.code = data1;
    evt.value = data2;
    return evt;
}

MidiEvent MidiEvent::fromPortMidiMessage(const PmEvent& midiData) {
    // no meaningful differences in received messages found, yet

    MidiEvent evt;

    int msgStatus = Pm_MessageStatus(midiData.message);
    evt.code = Pm_MessageData1(midiData.message);
    evt.value = Pm_MessageData2(midiData.message);

    int typeCode = msgStatus & 0xF0;
    evt.channel = (msgStatus & 0x0F) + 1;

    switch (typeCode) {
        case NOTEOFF:
            evt.type = MidiEventType::NoteOff;
            break;
        case NOTEON:
            evt.type = MidiEventType::NoteOn;
            break;
        case CTRLCHANGE:
            evt.type = MidiEventType::ControlChange;
            break;
        default:
            evt.type = MidiEventType::Unknown;
            break;
    }


    return evt;
}


bool MidiEvent::isValid() const {
    return type != MidiEventType::Unknown;
}









