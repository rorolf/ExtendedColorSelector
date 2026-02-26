



#include "EXMIDIMappingEngine.h"
#include <QtGlobal>

static inline int clamp01_127(int v) { return qBound(0, v, 127); }

void MidiMappingEngine::process(const MidiEvent& ev)
{
    // Expect ev to have: ev.eventTypeIndex (or similar), ev.code, ev.value 0..127.
    // If your MidiEvent uses different names, adapt here in one place.
    const int evType = ev.eventTypeIndex;  // e.g., 0 = CC
    const int code   = ev.code;            // controller number
    const int value  = clamp01_127(ev.value);

    for (const auto& e : m_mappings) {
        if (e.eventTypeIndex != evType) continue;
        if (e.eventCode != code) continue;

        switch (e.inputBehavior) {
            case InputBehavior::Knob:   handleKnob(e, value);   break;
            case InputBehavior::Button: handleButton(e, value); break;
            case InputBehavior::Switch: handleSwitch(e, value); break;
            default: break;
        }
    }
}

void MidiMappingEngine::handleKnob(const MappingEntry& e, int value)
{
    if (!m_bus) return;
    // For knobs, emit raw 0..127. Consumers can scale.
    m_bus->emitValue<int>(e.mappedAction, value);
}

void MidiMappingEngine::handleButton(const MappingEntry& e, int value)
{
    if (!m_bus) return;
    const bool active = value >= e.threshold;
    m_bus->emitValue<bool>(e.mappedAction, active);
}

void MidiMappingEngine::handleSwitch(const MappingEntry& e, int value)
{
    if (!m_bus) return;
    auto k = keyFor(e);
    auto& st = m_state[k]; // default-init if new

    const int thr = e.threshold;
    const int hyst = qMin(thr, e.hysteresis); // deadband, must not span out of signal range

    // A "switch" toggles only when crossing the threshold, with hysteresis in case the channel is noisy.
    // - Upward crossing (<=thr -> >thr): toggle ON if responsive, then become non-responsive
    //   until we exit the deadband downward below thr - hyst.
    // - Downward crossing (>thr -> <=thr): become responsive again after exiting deadband.

    const bool wasAbove = st.lastValue > thr;
    const bool isAbove  = value > thr;

    // Detect upward crossing
    if (!wasAbove && isAbove && st.responsive) {
        st.latched = !st.latched;  // toggle
        st.responsive = false;     // hold until we exit hysteresis band
        m_bus->emitValue<bool>(e.mappedAction, st.latched);
    }

    // Re-arm responsiveness when sufficiently below threshold
    if (value < thr - hyst) {
        st.responsive = true;
    }

    st.lastValue = value;
}





