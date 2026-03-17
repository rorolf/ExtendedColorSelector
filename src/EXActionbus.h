

#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QVariant>


#include "EXColorMixerDock.h"
#include "EXColorSelectorDock.h"
#include "EXMIDIEvent.h"
#include "EXMIDIMapper_PresetControl.h"
//#include "EXMIDIPanelWidget.h"
#include "EXColorMixState.h"
#include "EXColorPresetStore.h"
#include "EXSettingsState.h"

class EXActionBus : public QObject, public KisShared
{
    Q_OBJECT

    public:
        EXActionBus(QObject* parent = nullptr);
        ~EXActionBus() {};
        void initializeAndConnectTo(EXColorMixerDock* ui);
        void initializeAndConnectToEXS(EXColorSelectorDock* ui);
        static EXActionBus* instance();

        EXColorMixerDock* m_ui;
        EXColorSelectorDock* m_tmpui;
        //EXMIDIPanelWidgetSP m_midiUi;
        MidiListener* m_midiListener;
        EXMIDIMapperPresetControl* m_mapper;
        EXColorMixStateSP m_mixer;
        EXColorPresetStoreSP m_colorPresets;
        EXSettingsStateSP m_settingsState;

    public Q_SLOTS:
        void onMidiMessage(const MidiEvent& evt);
        void onRefreshMidiPorts();
        void onPortSelected(const QString& portName);

    Q_SIGNALS:
        void sigLogMessage(const QString& message) const;
        void sigInputPortsChanged(const QStringList& ports);
        void sigKnobTurned(const int knob, const int value);
        void sigPadPressed(const int knob, const int value);

    public:
        QStringList currentPorts;
        QString currentPortName;

    private:
        QTimer* portRefreshTimer;

};


