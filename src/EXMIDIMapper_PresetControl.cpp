


#include "EXMIDIEvent.h"
#include "EXMIDIMappingEntry.h"
#include "EXMIDIMapper_PresetControl.h"
#include <QtGlobal>
#include <cmath>
#include <qobjectdefs.h>

static inline int clamp0_127(int v) { return qBound(0, v, 127); }

    // EXActionBus decides how to propagate signals
MappedMidiEvent EXMIDIMapperPresetControl::mapMidiEvent(const MidiEvent& ev)
{
    const int value  = clamp0_127(ev.value);

    MappedMidiEvent mmEvt;
    for (const auto& e : m_mappings) {

        if (e.matchesEvent(ev)) {
            switch (e.inputBehavior) {
                case InputBehavior::Knob:   mapKnobEvent(mmEvt, e, value);   break;
                case InputBehavior::Button: mapButtonEvent(mmEvt, e, value); break;
                case InputBehavior::Switch: mapSwitchEvent(mmEvt, e, value); break;
                default: break;
            };
            return mmEvt;
        }
    }
    return mmEvt;
}

void EXMIDIMapperPresetControl::mapKnobEvent(MappedMidiEvent& mappedEvent, const MappingEntry& e, const int value)
{
    // if (e.mapsToKnob()) {
    //     emit sigKnobTurned(e.deviceIndex(), value);
    // }
    mappedEvent.mappedAction = e.mappedAction;
    mappedEvent.value = value;
    mappedEvent.ignoreEvent = false;
}

void EXMIDIMapperPresetControl::mapButtonEvent(MappedMidiEvent& mappedEvent, const MappingEntry& e, const int value)
{
    auto k = keyFor(e);
    auto& st = m_state[k];
    const bool nowActive = value >= e.threshold;
    const bool wasActive = st.latched;

    qDebug() << " Before: Button pressed with:" << value << "; it had been active:" << wasActive << ", and now:" << nowActive;

    mappedEvent.mappedAction = e.mappedAction;
    mappedEvent.value = (int)nowActive;
    if (wasActive != nowActive){
        st.latched = nowActive;
        // if (e.mapsToPad()) {
        //     emit sigPadPressed(e.deviceIndex(), st.latched);
        // }
        mappedEvent.ignoreEvent = false;
    } else {
        mappedEvent.ignoreEvent = true;
    }
}

void EXMIDIMapperPresetControl::mapSwitchEvent(MappedMidiEvent& mappedEvent, const MappingEntry& e, const int value)
{
    auto k = keyFor(e);
    auto& st = m_state[k];

    const int threshold = e.threshold;
    const int hysteresis = qMin(threshold-1, e.hysteresis); // deadband, must not span out of signal range

    // A "switch" toggles only when crossing the threshold, with hysteresis in case the channel is noisy.
    // This is the recommended way to interpret touchpads
    // - Upward crossing (<=thr -> >thr): toggle ON if responsive, then become non-responsive
    //   until we exit the deadband downward below thr - hyst.
    // - Downward crossing (>thr -> <=thr-hysteresis): become responsive again

    const bool isAboveThreshold  = value >= threshold;
    const bool isResponsive = st.responsive;
    const bool significantlyBelow = value <= qMax(threshold - hysteresis, 0);

    // Detect upward crossing
    if (isResponsive && isAboveThreshold) {
        st.latched = !st.latched;
        st.responsive = false;     // ignore small signal variations in case of noise
        // if (e.mapsToPad()) {
        //     emit sigPadPressed(e.deviceIndex(), st.latched);
        // }

        // QString switchStateMsg;
        // if (st.latched) { switchStateMsg = "Switch ON"; } else { switchStateMsg = "Switch OFF"; }
        // qDebug() << "New event of switch:" << switchStateMsg;
        mappedEvent.ignoreEvent = false;
    } else {
        // QString switchStateMsg;
        // if (st.latched) { switchStateMsg = "Switch ON"; } else { switchStateMsg = "Switch OFF"; }
        // qDebug() << "Repeat event of switch:" << switchStateMsg;
        mappedEvent.ignoreEvent = true;
    }

    if (!isResponsive && significantlyBelow) {
        st.responsive = true;
    }

    st.lastValue = value;
    mappedEvent.mappedAction = e.mappedAction;
    mappedEvent.value = (int)st.latched;
}

void EXMIDIMapperPresetControl::setMappings(const QVector<MappingEntry>& newMappings) {
    m_mappings = newMappings;
    m_state.clear();
}









