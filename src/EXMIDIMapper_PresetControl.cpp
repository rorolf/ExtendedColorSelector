


#include "EXMIDIEvent.h"
#include "EXMIDIMappingEntry.h"
#include "EXMIDIMapper_PresetControl.h"
#include <QtGlobal>
#include <cmath>
#include <qobjectdefs.h>

static inline int clamp0_127(int v) { return qBound(0, v, 127); }
typedef std::tuple<InputBehavior, int> MappedMidiEvent;

MappedMidiEvent EXMIDIMapperPresetControl::mapMidiEvent(const MidiEvent& ev)
{
    // Expect ev to have: ev.eventTypeIndex (or similar), ev.code, ev.value 0..127.
    // If your MidiEvent uses different names, adapt here in one place.
    const MidiEventType evType = ev.type;  // e.g., 0 = CC
    const int code   = ev.code;            // controller number
    const int value  = clamp0_127(ev.value);

    for (const auto& e : m_mappings) {
        if (e.eventType != evType) continue;
        if (e.eventCode != code) continue;

        if (e.matchesEvent(ev)) {
            switch (e.inputBehavior) {
                case InputBehavior::Knob:   return {InputBehavior::Knob, processKnob(e, value)};   break;
                case InputBehavior::Button: return {InputBehavior::Button, processButton(e, value)}; break;
                case InputBehavior::Switch: return {InputBehavior::Switch, processSwitch(e, value)}; break;
                default: break;
            };
        }
    }
    return {InputBehavior::Button,-1};
}

int EXMIDIMapperPresetControl::processKnob(const MappingEntry& e, int value)
{
    // if (e.mapsToKnob()) {
    //     emit sigKnobTurned(e.deviceIndex(), value);
    // }
    return value;
}

int EXMIDIMapperPresetControl::processButton(const MappingEntry& e, int value)
{
    auto k = keyFor(e);
    auto& st = m_state[k];
    const bool nowActive = value >= e.threshold;
    const bool wasActive = st.latched;
    if (wasActive != nowActive){
        st.latched = nowActive;
        // if (e.mapsToPad()) {
        //     emit sigPadPressed(e.deviceIndex(), st.latched);
        // }
        return (int)nowActive;
    } else {
        return -1;
    }
}

int EXMIDIMapperPresetControl::processSwitch(const MappingEntry& e, int value)
{
    // if (!m_bus) return;
    auto k = keyFor(e);
    auto& st = m_state[k]; // default-init if new

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
        st.latched = !st.latched;  // toggle
        st.responsive = false;     // ignore small signal variations in case of noise
        // m_bus->emitValue<bool>(e.mappedAction, st.latched);
        // if (e.mapsToPad()) {
        //     emit sigPadPressed(e.deviceIndex(), st.latched);
        // }
        return (int)st.latched;
    }

    if (!isResponsive && significantlyBelow) {
        st.responsive = true;
    }

    st.lastValue = value;

    return std::copysign((int)st.latched, -1);
}













