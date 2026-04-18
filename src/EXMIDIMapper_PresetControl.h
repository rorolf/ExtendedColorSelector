




#pragma once
#include <QObject>
#include <QHash>
#include <QVariant>
#include <QVector>
#include "EXMIDIMappingEntry.h"
#include "EXMIDIEvent.h"
#include <QDebug>

struct MappedMidiEvent
{
    public:
        EXMappedMidiAction mappedAction = EXMappedMidiAction::None;
        int value = -1;
        bool ignoreEvent = true;
};

// Minimal, self-contained mapping engine that owns mappings and emits to ActionBus.
class EXMIDIMapperPresetControl
{
    public:
        explicit EXMIDIMapperPresetControl() {}

        void setMappings(const QVector<MappingEntry>& m);

        // return -1
        MappedMidiEvent mapMidiEvent(const MidiEvent& ev);

    private:
        const QVector<MappingEntry>& mappings() const { return m_mappings; }

        struct SwitchState {
            bool latched = false;         // current on/off state exposed to consumers
            int lastValue = -1;           // last seen MIDI value (0..127)
            bool responsive = true;       // within hysteresis window, ignore re-triggers
        };

        using Key = QPair<int,int>;
        static Key keyFor(const MappingEntry& e) { return qMakePair((int)e.eventType, (int)e.eventCode); }

        // redundant wqith mappingentry::processInput
        void mapKnobEvent(MappedMidiEvent& mappedValue, const MappingEntry& e, const int value);
        void mapButtonEvent(MappedMidiEvent& mappedValue, const MappingEntry& e, const int value);
        void mapSwitchEvent(MappedMidiEvent& mappedValue, const MappingEntry& e, const int value);

    public Q_SIGNAL:
        void sigKnobTurned(int knob, int value);
        void sigPadPressed(int pad, bool isPressed);

    // public Q_SLOTS:
    //     void onMappingsUpdated(const QVector<MappingEntry>& newMappings);

    private:
        QVector<MappingEntry> m_mappings;
        QHash<Key, SwitchState> m_state;
};



