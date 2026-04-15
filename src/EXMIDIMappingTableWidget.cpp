

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QVariant>
#include <qabstractitemmodel.h>
#include <qlist.h>
#include <qobjectdefs.h>
#include <qsettings.h>

#include "EXMIDIMappingTableWidget.h"
#include "EXMIDIMapper_PresetControl.h"
#include "EXMIDIMappingEntry.h"
#include "EXMIDIEvent.h"

#include <QDebug>
#include <QObject>
#include <QMetaObject>
#include <QMetaMethod>

MappingTableWidget::MappingTableWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Port selection
    QHBoxLayout* portRow = new QHBoxLayout();
    QLabel* portLabel = new QLabel();
    portLabel->setText("Select MIDI Input Port:");

    this->portCombo = new QComboBox();
    this->connectionStatusLabel = new QLabel("Waiting...");
    connectionStatusLabel->setStyleSheet("color: orange;");

    connect(portCombo, &QComboBox::currentTextChanged, this, [this](const QString& name) {
        this->setDesiredPort(name);
    });

    portRow->addWidget(portLabel);
    portRow->addWidget(portCombo);
    portRow->addWidget(connectionStatusLabel);
    portRow->addStretch();
    layout->addLayout(portRow);

    // Table showing mappings
    table = new QTableWidget();
    table->setColumnCount(Column::ColumnCount);
    table->setHorizontalHeaderLabels({
        "Action", "Event Type", "Code", "Behavior", "Threshold", "Hysteresis", "Delete"
    });
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    table->setRowCount(0);
    for (EXMappedMidiAction action :AllEXMappedMidiActions()) {
        if (action == EXMappedMidiAction::None) continue;
        MappingEntry entry; entry.mappedAction = action;
        this->addMappingRow(entry);
    }

    layout->addWidget(table);

    QHBoxLayout* bottomRow = new QHBoxLayout();

    showLogCheckbox = new QCheckBox("Show MIDI Log", this);
    connect(showLogCheckbox, &QCheckBox::toggled, this, &MappingTableWidget::sigShowLogToggled);
    bottomRow->addWidget(showLogCheckbox);


    // bottomRow->addWidget(portCombo);
    // bottomRow->addWidget(connectionStatusLabel);

    bottomRow->addStretch();
    layout->addLayout(bottomRow);
}

void MappingTableWidget::overwriteWithMappings(const QList<MappingEntry>& entries) {
    int k = 0;
    for (const MappingEntry& entry : entries) {
        this->overwriteMappingRow(k++, entry);
    }
}

void MappingTableWidget::overwriteWithMappings(const QList<std::tuple<int, MappingEntry>>& entries) {
    for (auto[index, entry] : entries) {
        this->overwriteMappingRow(index, entry);
    }
}
void MappingTableWidget::addMappingRow(const MappingEntry& entry = MappingEntry::EmptyMappingEntry())
{
    int row = table->rowCount();
    table->insertRow(row);
    this->overwriteMappingRow(row, entry);
}

bool MappingTableWidget::overwriteMappingRow(int rowIndex, const MappingEntry& entry) {
    if (table->rowCount() > rowIndex+1) return false;

    QComboBox* maComboBox = mappedActionComboBox(entry.mappedAction);
    QComboBox* evTComboBo = eventTypeComboBox(entry.eventType);
    QSpinBox* mEvCSpinBox = midiEventCodeSpinBox(entry.eventCode);
    QComboBox* iBComboBox = inputBehaviorComboBox(entry.inputBehavior);
    QSpinBox* tSpinBox = thresholdSpinBox(entry.threshold, entry.inputBehavior);
    QSpinBox* hSpinBox = hysteresisSpinBox(entry.hysteresis, entry.inputBehavior);

    connect(maComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MappingTableWidget::onAnyWidgetChanged);
    connect(evTComboBo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,  &MappingTableWidget::onAnyWidgetChanged);
    connect(mEvCSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MappingTableWidget::onAnyWidgetChanged);
    connect(iBComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MappingTableWidget::onAnyWidgetChanged);
    connect(tSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MappingTableWidget::onAnyWidgetChanged);
    connect(hSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MappingTableWidget::onAnyWidgetChanged);

    table->setCellWidget(rowIndex, ActionColumn,     maComboBox);
    table->setCellWidget(rowIndex, EventTypeColumn,  evTComboBo);
    table->setCellWidget(rowIndex, CodeColumn,       mEvCSpinBox);
    table->setCellWidget(rowIndex, BehaviorColumn,   iBComboBox);
    table->setCellWidget(rowIndex, ThresholdColumn,  tSpinBox);
    table->setCellWidget(rowIndex, HysteresisColumn, hSpinBox);

    auto* deleteButton = new QPushButton("Delete");
    connect(deleteButton, &QPushButton::clicked, this, [this, rowIndex]() {
        this->emptyRow(rowIndex);
    });
    table->setCellWidget(rowIndex, DeleteColumn, deleteButton);
    // qDebug() << QString("DEBUG: table row %1 got overwritten").arg(QString::number(rowIndex));
    return true;
}

void MappingTableWidget::addRowsFromSettings(QSettings& settings)
{
    Q_UNUSED(settings);
    // TODO: read mappings from settings
    // this->overwriteWithMappings(mappings);
}

void MappingTableWidget::saveMappingsToSettings(QSettings& settings) const {
    Q_UNUSED(settings);
    //QVector<MappingEntry> mappings = MappingTableWidget::collectMappingsFromTable();
}

QVector<MappingEntry> MappingTableWidget::collectMappingsFromTable() const
{
    QVector<MappingEntry> out;
    out.reserve(table->rowCount());

    // qDebug() << QString("Number of mappings: %1").arg(QString::number(out.length()));

    for (int row = 0; row < table->rowCount(); ++row) {
        // qDebug() << QString("Processing row: %1").arg(QString::number(row));
        QComboBox* mappedActionComboBox  = qobject_cast<QComboBox*>(table->cellWidget(row, ActionColumn));
        QComboBox* eventTypeComboBox     = qobject_cast<QComboBox*>(table->cellWidget(row, EventTypeColumn));
        QSpinBox* eventCodeSpinBox       = qobject_cast<QSpinBox*>(table->cellWidget(row, CodeColumn));
        QComboBox* inputBehaviorComboBox = qobject_cast<QComboBox*>(table->cellWidget(row, BehaviorColumn));
        QSpinBox* thresholdSpinBox       = qobject_cast<QSpinBox*>(table->cellWidget(row, ThresholdColumn));
        QSpinBox* hysteresisSpinbox      = qobject_cast<QSpinBox*>(table->cellWidget(row, HysteresisColumn));

        MappingEntry e;

        if (!mappedActionComboBox) qDebug() << "mappedActionComboBox is null";
        if (!eventTypeComboBox) qDebug() << "eventTypeComboBox is null";
        if (!eventCodeSpinBox) qDebug() << "eventCodeSpinBox is null";
        if (!inputBehaviorComboBox) qDebug() << "inputBehaviorComboBox is null";
        if (!thresholdSpinBox) qDebug() << "thresholdSpinBox is null";
        if (!hysteresisSpinbox) qDebug() << "hysteresisSpinbox is null";

        e.mappedAction = mappedActionComboBox ? mappedActionComboBox->currentData().value<EXMappedMidiAction>() :
                                                EXMappedMidiAction::None;
        e.eventType = eventTypeComboBox ? eventTypeComboBox->currentData().value<MidiEventType>() :
                                          MidiEventType::Unknown;

        e.eventCode = eventCodeSpinBox ? eventCodeSpinBox->value() : 0;

        if (inputBehaviorComboBox) {
            e.inputBehavior = inputBehaviorComboBox->currentData().value<InputBehavior>();
        } else {
            e.inputBehavior = InputBehavior::Knob;
        }
        e.threshold  = thresholdSpinBox  ? thresholdSpinBox->value()  : 64;
        e.hysteresis = hysteresisSpinbox ? hysteresisSpinbox->value() : 10;

        // qDebug() << QString("Mapped Action: %1").arg(EXMappedMidiActionToString(e.mappedAction));
        // qDebug() << QString("Event Type: %1").arg(midiEventTypeToString(e.eventType));
        // qDebug() << QString("Event Code: %1").arg(QString::number(e.eventCode));
        // qDebug() << QString("Input Behavior: %1").arg(inputBehaviorToString(e.inputBehavior));
        // qDebug() << QString("Threshold: %1").arg(QString::number(e.threshold));
        // qDebug() << QString("Hysteresis: %1").arg(QString::number(e.hysteresis));

        out.push_back(e);
    }

    // qDebug() << "Reading table contents succeeded!";
    return out;
}

void MappingTableWidget::hookRowWidgets(int row)
{
    auto hook = [&](QWidget* w) {
        if (!w) return;
        connect(w, SIGNAL(destroyed(QObject*)), this, SLOT(update()));
        if (auto* cb = qobject_cast<QComboBox*>(w)) {
            connect(cb, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, [this](int value){ Q_UNUSED(value); onAnyWidgetChanged(); });
        } else if (auto* sb = qobject_cast<QSpinBox*>(w)) {
            connect(sb, QOverload<int>::of(&QSpinBox::valueChanged),
                    this, [this](int value){ Q_UNUSED(value); onAnyWidgetChanged(); });
        }
    };

    for (int col = 0; col < table->columnCount(); ++col) {
        hook(table->cellWidget(row, col));
    }
}

void MappingTableWidget::onAnyWidgetChanged()
{

    // Re-apply enable/disable for threshold/hysteresis based on behavior of each row
    for (int row = 0; row < table->rowCount(); ++row) {
        QComboBox* inputBehaviorComboBox  = qobject_cast<QComboBox*>(table->cellWidget(row, BehaviorColumn));
        QSpinBox* thresholdSpinBox  = qobject_cast<QSpinBox*>(table->cellWidget(row, ThresholdColumn));
        QSpinBox* hysteresisSpinbox = qobject_cast<QSpinBox*>(table->cellWidget(row, HysteresisColumn));
        if (!inputBehaviorComboBox) continue;
        const auto beh = inputBehaviorComboBox->currentData().value<InputBehavior>();
        const bool thrOn  = (beh == InputBehavior::Button) || (beh == InputBehavior::Switch);
        const bool hystOn = (beh == InputBehavior::Switch);
        if (thresholdSpinBox) {
            thresholdSpinBox->setEnabled(thrOn);
        }
        if (hysteresisSpinbox) {
            hysteresisSpinbox->setEnabled(hystOn);
        }
    }

    QVector<MappingEntry> currentMappings = collectMappingsFromTable();
    emit sigMappingsEdited(currentMappings);
}

void MappingTableWidget::onMidiDeviceChanged(const QString& deviceName) {
    if (deviceName == NULL || deviceName.isEmpty()) {
        this->connectionStatusLabel->setText(QString("No connection"));
        connectionStatusLabel->setStyleSheet("color: red;");
    } else if (deviceName == this->desiredPortName) {
        this->setConnectionStatus(true);
    } else {
        this->connectionStatusLabel->setText(QString("Connected to %1 instead").arg(deviceName));
        connectionStatusLabel->setStyleSheet("color: red;");
    }
}




void MappingTableWidget::emptySelectedRows()
{
    QModelIndexList selected = table->selectionModel()->selectedRows();
    for (const QModelIndex& idx : selected) {
        this->emptyRow(idx);
    }
}

void MappingTableWidget::emptyRow(QModelIndex idx)
{
    this->emptyRow(idx.row());
}

void MappingTableWidget::emptyRow(int row)
{
    MappingEntry nullEntry = MappingEntry::EmptyMappingEntry();
    auto actionComboBox = qobject_cast<QComboBox*>(table->cellWidget(row, ActionColumn));
    if (actionComboBox) {
        auto ind = actionComboBox->findData(QVariant::fromValue(nullEntry.mappedAction));
        if (ind>=0) {
            actionComboBox->blockSignals(true);
            actionComboBox->setCurrentIndex(ind);
            actionComboBox->blockSignals(false);
        }
    }

    auto eventTComboBox = qobject_cast<QComboBox*>(table->cellWidget(row, EventTypeColumn));
    if (eventTComboBox) {
        auto ind = eventTComboBox->findData(QVariant::fromValue(nullEntry.eventType));
        if (ind>=0) {
            actionComboBox->blockSignals(true);
            eventTComboBox->setCurrentIndex(ind);
            actionComboBox->blockSignals(false);
        }
    }

    QSpinBox* eCodeSpinBox  = qobject_cast<QSpinBox*>(table->cellWidget(row, CodeColumn));
    if (eCodeSpinBox) eCodeSpinBox->setValue(nullEntry.eventCode);

    auto behaviorComboBox = qobject_cast<QComboBox*>(table->cellWidget(row, BehaviorColumn));
    if (behaviorComboBox) {
        auto ind = behaviorComboBox->findData(QVariant::fromValue(nullEntry.inputBehavior));
        if (ind>=0) {
            actionComboBox->blockSignals(true);
            behaviorComboBox->setCurrentIndex(ind);
            actionComboBox->blockSignals(false);
        }
    }

    QSpinBox* thresholdSpinBox  = qobject_cast<QSpinBox*>(table->cellWidget(row, ThresholdColumn));
    if (thresholdSpinBox) {
        actionComboBox->blockSignals(true);
        thresholdSpinBox->setValue(nullEntry.threshold);
        actionComboBox->blockSignals(false);
    }
    QSpinBox* hysteresisSpinbox = qobject_cast<QSpinBox*>(table->cellWidget(row, HysteresisColumn));
    if (hysteresisSpinbox) {
        actionComboBox->blockSignals(true);
        hysteresisSpinbox->setValue(nullEntry.hysteresis);
        actionComboBox->blockSignals(false);
    }

    emit sigMappingChanged(row);
}




void MappingTableWidget::setPortList(const QStringList& ports)
{
    // portCombo->blockSignals(true);
    // portCombo->clear();
    // portCombo->addItems(ports);
    // portCombo->blockSignals(false);

    this->setAvailablePorts(ports);
}

QString MappingTableWidget::currentPortName() const
{
    return portCombo->currentText();
}

void MappingTableWidget::setCurrentPortIndex(int index)
{
    portCombo->setCurrentIndex(index);
}


QStringList MappingTableWidget::availablePorts()
{
    return currentPorts;
}

void MappingTableWidget::setAvailablePorts(const QStringList& ports)
{
    currentPorts = ports;
    portCombo->blockSignals(true);
    portCombo->clear();
    portCombo->addItems(ports);
    portCombo->blockSignals(false);

    bool resetDesiredPortName = true;
    for (auto device : this->currentPorts) {
        if (device == this->desiredPortName) {
            resetDesiredPortName = false;
            break;
        }
    }
    if (ports.length()>0 && resetDesiredPortName) { this->setDesiredPort(ports[0]); }
}

void MappingTableWidget::setDesiredPort(const QString& port)
{
    desiredPortName = port;
    this->setConnectionStatus(false);
    emit sigPortSelected(port);
}

QString MappingTableWidget::desiredPort() const
{
    return desiredPortName;
}

void MappingTableWidget::setConnectionStatus(bool connected)
{
    isConnected = connected;

    if (connected) {
        connectionStatusLabel->setText(QString("Connected to %1").arg(desiredPortName));
        connectionStatusLabel->setStyleSheet("color: green;");
    } else {
        connectionStatusLabel->setText(QString("Waiting for %1...").arg(desiredPortName));
        connectionStatusLabel->setStyleSheet("color: orange;");
    }
}



//##############################################################################
//## UI elements for individual rows
//##############################################################################

QComboBox* MappingTableWidget::mappedActionComboBox(EXMappedMidiAction initalAction)
{
    auto* combo = new QComboBox();

    const std::array<const EXMappedMidiAction, 17> itemData = AllEXMappedMidiActions();
    const std::array<const QString, 17> itemTexts = EXMappedMidiActionTexts();
    int initialIndex=0;
    for (size_t k=0; k<itemData.size(); ++k) {
        combo->addItem(itemTexts[k], QVariant::fromValue(itemData[k]));
        if (initalAction == itemData[k])
            initialIndex=k;
    }

    combo->setCurrentIndex(initialIndex);
    return combo;
}


QComboBox* MappingTableWidget::eventTypeComboBox(MidiEventType initalEventType)
{
    auto* combo = new QComboBox();

    const std::array<const MidiEventType, 4> eventTypes = AllMidiEventTypes();
    const std::array<const QString, 4> eventTypeTexts = AllMidiEventTypeTexts();
    int initialIndex=0;
    for (size_t k=0; k<eventTypes.size(); ++k) {
        if (eventTypes[k] != MidiEventType::Unknown) {
            combo->addItem(eventTypeTexts[k], QVariant::fromValue(eventTypes[k]));
            if (initalEventType == eventTypes[k])
                initialIndex=k;
        }
    }
    combo->setCurrentIndex(initialIndex);
    return combo;
}

QSpinBox* MappingTableWidget::midiEventCodeSpinBox(int initialValue)
{
    auto* spin = new QSpinBox();
    spin->setRange(0, 127);
    spin->setValue(initialValue);
    return spin;
}

QComboBox* MappingTableWidget::inputBehaviorComboBox(InputBehavior initialBehavior)
{
    auto* comboBox = new QComboBox();

    for (InputBehavior beh : {InputBehavior::Knob, InputBehavior::Button, InputBehavior::Switch}) {
        comboBox->addItem(inputBehaviorToString(beh), QVariant::fromValue(beh));
    }

    const int idx = comboBox->findData(QVariant::fromValue(initialBehavior));
    if (idx >= 0) { comboBox->setCurrentIndex(idx); }
    else { comboBox->setCurrentIndex(0); }

    return comboBox;
}

QSpinBox* MappingTableWidget::thresholdSpinBox(int initialValue, InputBehavior behavior)
{
    auto* spin = new QSpinBox();
    spin->setRange(0, 127);
    spin->setValue(initialValue);

    // Threshold is meaningful for Button and Switch, not for Knob
    const bool needsThreshold =
        (behavior == InputBehavior::Button) || (behavior == InputBehavior::Switch);
    spin->setEnabled(needsThreshold);

    return spin;
}

QSpinBox* MappingTableWidget::hysteresisSpinBox(int initialValue, InputBehavior behavior)
{
    auto* spin = new QSpinBox();
    spin->setRange(0, 127);
    spin->setValue(initialValue);

    // Hysteresis is only meaningful for Switch
    const bool needsHysteresis = (behavior == InputBehavior::Switch);
    spin->setEnabled(needsHysteresis);

    return spin;
}














