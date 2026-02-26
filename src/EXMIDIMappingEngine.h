




#pragma once
#include <QObject>
#include <QHash>
#include <QVariant>
#include <QVector>
#include "EXMIDIMappingEntry.h"
#include "EXMIDIEvent.h"
#include "EXMIDIActionBus.h"

// Minimal, self-contained mapping engine that owns mappings and emits to ActionBus.
class MidiMappingEngine : public QObject {
    Q_OBJECT
public:
    explicit MidiMappingEngine(ActionBus* bus, QObject* parent = nullptr)
        : QObject(parent), m_bus(bus) {}

    void setMappings(const QVector<MappingEntry>& m) { m_mappings = m; m_state.clear(); }
    const QVector<MappingEntry>& mappings() const { return m_mappings; }

public Q_SLOTS:
    // Preferred: connect this to MidiListener's event signal.
    void process(const MidiEvent& ev);

private:
    struct SwitchState {
        bool latched = false;         // current on/off state exposed to consumers
        int lastValue = -1;           // last seen MIDI value (0..127)
        bool responsive = true;       // within hysteresis window, ignore re-triggers
    };

    // Key state per (eventType, code). If your MappingEntry uses eventTypeIndex + eventCode, we mirror that.
    using Key = QPair<int,int>;
    static Key keyFor(const MappingEntry& e) { return Key{e.eventType, e.code}; }

    void handleKnob(const MappingEntry& e, int value);
    void handleButton(const MappingEntry& e, int value);
    void handleSwitch(const MappingEntry& e, int value);

private:
    QVector<MappingEntry> m_mappings;
    QHash<Key, SwitchState> m_state;
    ActionBus* m_bus = nullptr; // not owned
};




