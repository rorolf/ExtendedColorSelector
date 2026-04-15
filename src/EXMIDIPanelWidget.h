#pragma once

#include <QSettings>
#include <QMainWindow>
#include <QComboBox>
#include <QDockWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QTimer>
#include <QCheckBox>

#include <kis_shared.h>
#include <kis_shared_ptr.h>

#include "EXMIDILogPanelWidget.h"
#include "EXMIDIMappingTableWidget.h"
#include "EXMIDIListener.h"
#include "EXMIDIEvent.h"
#include "EXMIDIMappingEntry.h"
#include "EXMIDIMapper_PresetControl.h"

class EXMIDIPanelWidget : public QWidget
{
    Q_OBJECT

public:
    EXMIDIPanelWidget(QWidget* parent = nullptr);
    ~EXMIDIPanelWidget();

public Q_SLOTS:
    void onPortsAvailable();
    void onPortSelected();
    void onMidiMessage(const MidiEvent& evt);
    void onError(const QString& message);
    void toggleLogVisibility(bool checked);
    void onLogCheckboxToggled(bool checked);

public:
    void loadSettings();
    void saveSettings();

    //MidiListener* m_midiListener;
    //EXMIDIMapperPresetControl* m_mapper;

    // void tryAutoConnectToDesiredPort();
    // void refreshMidiPorts();

    MappingTableWidget* mappingTable;
    QString pendingReconnectPortName;

    LogPanelWidget* logPanel;
    QDockWidget* logDock;

    // QStringList currentPorts;
    // QString currentPortName;
    // QTimer* portRefreshTimer;

    // QList<MappingEntry> mappings;
};

typedef KisSharedPtr<EXMIDIPanelWidget> EXMIDIPanelWidgetSP;

