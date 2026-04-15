
#include <QSettings>
#include "EXMIDIPanelWidget.h"
#include "EXActionbus.h"
#include "EXMIDIMapper_PresetControl.h"
#include "EXMIDIMappingEntry.h"
#include "EXMIDIEvent.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QTimer>
#include <QMenuBar>
#include <QCheckBox>
#include <cmath>

EXMIDIPanelWidget::EXMIDIPanelWidget(QWidget* parent)
    : QWidget(parent)
{
    //################################################################################
    //## code that stays here
    //################################################################################

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    mappingTable = new MappingTableWidget(this);
    this->setMinimumSize(500, 400);
    this->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    setWindowTitle("Qt6 MIDI Listener");

    mainLayout->addWidget(mappingTable);

    // // Connect signals from the mapping widget
    // connect(mappingTable, &MappingTableWidget::portSelected, this, &EXMIDIPanelWidget::onPortSelected);
    // //connect(mappingTable, &MappingTableWidget::addMappingRequested, this, &EXMIDIPanelWidget::onAddMappingClicked);
    // connect(mappingTable, &MappingTableWidget::showLogToggled, this, &EXMIDIPanelWidget::onLogCheckboxToggled);
    // connect(mappingTable, &MappingTableWidget::mappingsEdited, this, &EXMIDIPanelWidget::onMappingsEdited);

    logPanel = new LogPanelWidget();
    mainLayout->addWidget(logPanel);
}

EXMIDIPanelWidget::~EXMIDIPanelWidget() {
    saveSettings();
}

void EXMIDIPanelWidget::loadSettings()
{
    qDebug() << "Loading Mappings from file...";
    QSettings settings("KritaExtension", "MidiGuiListener");

    pendingReconnectPortName = settings.value("selectedPort").toString();

    // No device is selected yet, just populate the list
    const QStringList currentPorts = EXActionBus::instance()->currentPorts;
    mappingTable->setPortList(currentPorts);


    QString savedPort = settings.value("selectedPort").toString();
    mappingTable->setDesiredPort(savedPort);

    QList<std::tuple<int, MappingEntry>> loadedMappings = QList<std::tuple<int, MappingEntry>>();
    int mappingCount = settings.beginReadArray("mappings");
    for (int k = 0; k < mappingCount; ++k) {
        settings.setArrayIndex(k);
        MappingEntry entry;
        entry.eventType = midiEventTypeFromString(settings.value("eventType").toString());
        entry.eventCode = settings.value("code").toInt();
        entry.inputBehavior = inputBehaviorFromString(settings.value("behavior").toString());
        entry.threshold = settings.value("threshold").toInt();
        entry.mappedAction = EXMappedMidiActionFromString(settings.value("mappedAction").toString());
        loadedMappings.append({k, entry});

        qDebug() << "Loaded settings for row" << QString::number(k)
                << EXMappedMidiActionToString(entry.mappedAction)
                << midiEventTypeToString(entry.eventType)
                << QString::number(entry.eventCode)
                << inputBehaviorToString(entry.inputBehavior)
                << QString::number(entry.threshold)
                << QString::number(entry.hysteresis)
        ;

    }
    settings.endArray();

    mappingTable->overwriteWithMappings(loadedMappings);
}

void EXMIDIPanelWidget::saveSettings()
{
    qDebug() << "Saving Mappings to file...";
    QSettings settings("KritaExtension", "MidiGuiListener");

    settings.setValue("selectedPort", mappingTable->desiredPort());

    settings.beginWriteArray("mappings");
    QVector<MappingEntry> mappings = this->mappingTable->collectMappingsFromTable();
    for (int i = 0; i < mappings.size(); ++i) {
        MappingEntry entry = mappings[i];
        qDebug() << "Saving settings for row" << QString::number(i)
                << EXMappedMidiActionToString(entry.mappedAction)
                << midiEventTypeToString(entry.eventType)
                << QString::number(entry.eventCode)
                << inputBehaviorToString(entry.inputBehavior)
                << QString::number(entry.threshold)
                << QString::number(entry.hysteresis)
        ;

        settings.setArrayIndex(i);
        settings.setValue("eventType", midiEventTypeToString(mappings[i].eventType));
        settings.setValue("code", mappings[i].eventCode);
        settings.setValue("behavior", inputBehaviorToString(mappings[i].inputBehavior));
        settings.setValue("threshold", mappings[i].threshold);
        settings.setValue("mappedAction", EXMappedMidiActionToString(mappings[i].mappedAction));
    }
    settings.endArray();
    qDebug() << "Saving Mappings to File concluded";
}


void EXMIDIPanelWidget::toggleLogVisibility(bool checked)
{
    if (checked)
        logDock->show();
    else
        logDock->hide();
}

void EXMIDIPanelWidget::onLogCheckboxToggled(bool checked)
{
    logDock->setVisible(checked);
}

void EXMIDIPanelWidget::onPortsAvailable() {
    const QStringList allPorts = EXActionBus::instance()->currentPorts;
    mappingTable->setAvailablePorts(allPorts);
}

void EXMIDIPanelWidget::onPortSelected()
{
    const QString portName = EXActionBus::instance()->currentPortName;
    logPanel->appendLine(QString("[Connecting to port %1]").arg(portName));
    this->mappingTable->setDesiredPort(portName);
}

void EXMIDIPanelWidget::onError(const QString& message)
{
    QMessageBox::critical(this, "MIDI Error", message);
}
