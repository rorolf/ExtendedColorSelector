
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
    //setCentralWidget(mappingTable);
    //resize(500, 400);
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

    // logDock = new QDockWidget("MIDI Log");
    //logDock->setFeatures(QDockWidget::NoDockWidgetFeatures);
    // logDock->setTitleBarWidget(new QWidget());
    // logDock->setAllowedAreas(Qt::RightDockWidgetArea);
    // logDock->setMinimumWidth(logDock->parentWidget()->minimumWidth());
    // logDock->setMaximumWidth(logDock->parentWidget()->maximumWidth());
    // logDock->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    // logDock->setWidget(logPanel);
    // addDockWidget(Qt::RightDockWidgetArea, logDock);
    mainLayout->addWidget(logPanel);
    // logDock->hide();
    //
    currentPorts = EXActionBus::instance()->m_midiListener->availableInputPorts();
    mappingTable->setAvailablePorts(currentPorts);
    currentPortName = mappingTable->currentPortName();

    // connect(mappingTable, &MappingTableWidget::portSelected, this, [this](const QString& portName) {
    //     int idx = mappingTable->availablePorts().indexOf(portName);
    //     if (idx != -1) {
    //         m_midiListener->openPort(idx);
    //         currentPortName = portName;
    //         logPanel->appendLine(QString("[Connected to port %1]").arg(portName));
    //         mappingTable->setConnectionStatus(true);
    //     } else {
    //         mappingTable->setConnectionStatus(false);
    //     }
    // });


    loadSettings();
}

EXMIDIPanelWidget::~EXMIDIPanelWidget() {
    saveSettings();
}

void EXMIDIPanelWidget::loadSettings()
{
    QSettings settings("KritaExtension", "MidiGuiListener");

    pendingReconnectPortName = settings.value("selectedPort").toString();

    // No device is selected yet — just populate list
    mappingTable->setPortList(currentPorts);

    QString savedPort = settings.value("selectedPort").toString();
    mappingTable->setDesiredPort(savedPort);
    mappingTable->setConnectionStatus(false);

    mappingTable->addRowsFromSettings(settings);


    QList<std::tuple<int, MappingEntry>>* loadedMappings = new QList<std::tuple<int, MappingEntry>>();
    int mappingCount = settings.beginReadArray("mappings");
    for (int k = 0; k < mappingCount; ++k) {
        settings.setArrayIndex(k);
        MappingEntry entry;
        entry.eventType = midiEventTypeFromString(settings.value("eventType").toString());
        entry.eventCode = settings.value("code").toInt();
        entry.inputBehavior = inputBehaviorFromString(settings.value("behavior").toString());
        entry.threshold = settings.value("threshold").toInt();
        entry.mappedAction = EXMappedMidiActionFromString(settings.value("mappedAction").toString());
        mappings.append(entry);
        loadedMappings->append({k, entry});
    }
    settings.endArray();

    mappingTable->overwriteWithMappings(*loadedMappings);
}

void EXMIDIPanelWidget::saveSettings()
{
    QSettings settings("KritaExtension", "MidiGuiListener");

    settings.setValue("selectedPort", mappingTable->desiredPort());

    settings.beginWriteArray("mappings");
    for (int i = 0; i < mappings.size(); ++i) {
        settings.setArrayIndex(i);
        settings.setValue("eventType", midiEventTypeToString(mappings[i].eventType));
        settings.setValue("code", mappings[i].eventCode);
        settings.setValue("behavior", inputBehaviorToString(mappings[i].inputBehavior));
        settings.setValue("threshold", mappings[i].threshold);
        settings.setValue("mappedAction", EXMappedMidiActionToString(mappings[i].mappedAction));
    }
    settings.endArray();
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

void EXMIDIPanelWidget::onPortsAvailable(const QStringList& allPorts) {
    this->currentPorts = allPorts;
    mappingTable->setAvailablePorts(currentPorts);
    mappingTable->setCurrentPortIndex(0);
}

void EXMIDIPanelWidget::onPortSelected(const QString& portName)
{
    int index = currentPorts.indexOf(portName);
    if (index != -1) {
        //m_midiListener->openPort(index);
        currentPortName = portName;
        logPanel->appendLine(QString("[Connected to port %1]").arg(portName));
        mappingTable->setConnectionStatus(true);
    }
}

void EXMIDIPanelWidget::onMidiMessage(const MidiEvent& evt)
{
    // auto [inputType, value] = m_mapper->mapMidiEvent(evt);
    // if (std::signbit(value)==0) {
    //     switch (inputType) {
    //         case InputBehavior::Knob: break;
    //         case InputBehavior::Button: break;
    //         case InputBehavior::Switch: break;
    //     }
    // }
}


void EXMIDIPanelWidget::onError(const QString& message)
{
    QMessageBox::critical(this, "MIDI Error", message);
}
