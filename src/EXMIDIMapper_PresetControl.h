




#pragma once
#include <QObject>
#include <QHash>
#include <QVariant>
#include <QVector>
#include "EXMIDIMappingEntry.h"
#include "EXMIDIEvent.h"


typedef std::tuple<InputBehavior, int> MappedMidiEvent;

// Minimal, self-contained mapping engine that owns mappings and emits to ActionBus.
class EXMIDIMapperPresetControl
{
    public:
        explicit EXMIDIMapperPresetControl() {}

        void setMappings(const QVector<MappingEntry>& m) { m_mappings = m; m_state.clear(); }

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
        int processKnob(const MappingEntry& e, int value);
        int processButton(const MappingEntry& e, int value);
        int processSwitch(const MappingEntry& e, int value);

    public Q_SIGNAL:
        void sigKnobTurned(int knob, int value);
        void sigPadPressed(int pad, bool value);

    private:
        QVector<MappingEntry> m_mappings;
        QHash<Key, SwitchState> m_state;
};



