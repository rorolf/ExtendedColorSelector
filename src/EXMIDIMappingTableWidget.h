#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QPushButton>
#include <QCheckBox>
#include <QComboBox>
#include <QSpinBox>
#include <QLabel>
#include "EXMIDIEvent.h"
#include "EXMIDIMappingEntry.h"
#include <QSettings>


class QSettings;

enum Column {
    ActionColumn = 0,
    EventTypeColumn,
    CodeColumn,
    BehaviorColumn,
    ThresholdColumn,
    HysteresisColumn,
    DeleteColumn,
    ColumnCount
};


class MappingTableWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MappingTableWidget(QWidget* parent = nullptr);


    void addRowsFromSettings(QSettings& settings);
    QVector<MappingEntry> collectMappingsFromTable() const;
    void saveMappingsToSettings(QSettings& settings) const;
    void overwriteWithMappings(const QList<MappingEntry>& entries);
    void overwriteWithMappings(const QList<std::tuple<int, MappingEntry>>& entries);

    void addMappingRow(const MappingEntry& entry);
    bool overwriteMappingRow(int rowIndex, const MappingEntry& entry);
    void emptySelectedRows();
    void emptyRow(QModelIndex idx);
    void emptyRow(int row);

    void setPortList(const QStringList& ports);
    QString currentPortName() const;
    void setCurrentPortIndex(int index);

    QStringList availablePorts();
    void setAvailablePorts(const QStringList& ports);
    void setDesiredPort(const QString& portName);
    QString desiredPort() const;
    void setConnectionStatus(bool connected);

    QComboBox* mappedActionComboBox(EXMappedMidiAction initialAction);
    QComboBox* eventTypeComboBox(MidiEventType initialEventType);
    QSpinBox* midiEventCodeSpinBox(int initialValue);
    QComboBox* inputBehaviorComboBox(InputBehavior initialBehavior);
    QSpinBox* thresholdSpinBox(int initialValue, InputBehavior behavior);
    QSpinBox* hysteresisSpinBox(int initialValue, InputBehavior behavior);


Q_SIGNALS:
    void sigShowLogToggled(bool enabled);
    void sigPortSelected(const QString& name);
    void sigMappingChanged(int row);
    void sigMappingsEdited(const QVector<MappingEntry>&);

public Q_SLOTS:
    void onAnyWidgetChanged();

private:
    QTableWidget* table;
    QCheckBox* showLogCheckbox;
    QPushButton* addMappingButton;

    QComboBox* portCombo;
    QLabel* connectionStatusLabel;
    QLabel* debugLabel;

    QStringList currentPorts;
    QString desiredPortName;
    bool isConnected = false;

    void hookRowWidgets(int row);
    void debugSignals(QObject* obj, QString& identifier);
};
