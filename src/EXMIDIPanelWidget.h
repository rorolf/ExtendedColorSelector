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
#include "EXMIDIEvent.h"

class EXMIDIPanelWidget : public QWidget
{
    Q_OBJECT

public:
    EXMIDIPanelWidget(QWidget* parent = nullptr);
    ~EXMIDIPanelWidget();

public Q_SLOTS:
    void onPortsAvailable();
    void onPortSelected();
    void onError(const QString& message);
    void onLogCheckboxToggled(bool checked);

public:
    void loadSettings();
    void saveSettings();

    MappingTableWidget* mappingTable;
    QString pendingReconnectPortName;

    LogPanelWidget* logPanel;
};

typedef KisSharedPtr<EXMIDIPanelWidget> EXMIDIPanelWidgetSP;

