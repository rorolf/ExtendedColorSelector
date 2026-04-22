




#include "EXActionbus.h"
#include "EXColorMixState.h"
#include "EXColorMixerDock.h"
#include "EXColorModel.h"
#include "EXColorPresetStore.h"
#include "EXColorSelectorDock.h"
#include "EXMIDIEvent.h"
#include "EXMIDIListener.h"
#include "EXMIDIMapper_PresetControl.h"
#include "EXMIDIMappingEntry.h"
#include "EXMIDIMappingTableWidget.h"
#include <qglobal.h>


static EXActionBus *s_instance = nullptr;

EXActionBus* EXActionBus::instance()
{
    if (!s_instance) {
        s_instance = new EXActionBus();
    }
    return s_instance;
}

EXActionBus::EXActionBus(QObject*parent)
    : QObject(parent)
    , m_ui(nullptr)
    , m_tmpui(nullptr)
    //, m_midiUi(ui->m_midiPanel)
    , m_midiListener(new MidiListener)
    , m_mapper(new EXMIDIMapperPresetControl)
    , m_mixer(EXColorMixState::instance())
    , m_colorPresets(EXColorPresetStore::instance())
    , m_settingsState(EXSettingsState::instance())
{}

void EXActionBus::initializeAndConnectToEXS(EXColorSelectorDock* ui) {
    m_tmpui = ui;

    EXColorSelectorDock* uiCapture = m_tmpui;
    EXColorMixStateSP mixerCapture = m_mixer;
    QComboBox* presetSelector = uiCapture->m_presetSelector;
    QComboBox* cSS = uiCapture->m_colorSpaceSelector2;
    LogPanelWidget* logPanelCapture = m_tmpui->m_midiPanel->logPanel;

    connect(m_mixer.data(), &EXColorMixState::sigKritaBaseColorChanged, this, &EXActionBus::onKritaBaseColorChanged);

    connect(m_mixer.data(), &EXColorMixState::sigColorChanged, uiCapture, [uiCapture, mixerCapture](QVector3D newClr) {
        if (uiCapture->selectedMixChannel() < 0) {
            QColor newQClr = mixerCapture->toQColor(newClr);
            uiCapture->m_colorPatchPopup->updateColor(newQClr);
            uiCapture->m_mixResultColorPatch->m_color = newQClr;
            uiCapture->m_mixResultColorPatch->update();
        }
    });

    connect(cSS, QOverload<int>::of(&QComboBox::currentIndexChanged),
        uiCapture, [this, uiCapture, cSS](int newIndex) {
            Q_UNUSED(uiCapture);
            // auto data = cSS->itemData(newIndex);
            // if (data.isValid())
            // {
            //     ColorModelId newClrId = static_cast<ColorModelId>(data.value<int>());
            //     this->m_mixer->setColorModel(newClrId);
            // }
            ColorModelId newClrId = cSS->currentData().value<ColorModelId>();
            this->m_mixer->setColorModel(newClrId);
        }
    );

    connect(presetSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int newIndex) {
        qDebug() << "A new preset was selected via UI";
        m_colorPresets->onPresetSelected(newIndex);
        m_mixer->onColorPresetChanged(newIndex);
        this->m_tmpui->loadColorsFromPreset(newIndex);
    });

    //TODO: Change ColorModel when ColorSpace changes
    connect(m_mixer.data(), &EXColorMixState::sigColorSpaceChanged, uiCapture, [this, uiCapture, cSS](const KoColorSpace *colorSpace) {

        qDebug() << "EXColorMixState reported that the color space changed";
        auto newColorModel = ColorModelFactory::fromKoColorSpace(colorSpace);
        ColorModelId newClrId = newColorModel->id();
        delete newColorModel;
        ColorModelId oldClrId = uiCapture->m_colorSpaceSelector2->currentData().value<ColorModelId>();
        if (newClrId != oldClrId) {
            int newIndex = cSS->findData(newClrId);
            if (newIndex >= 0)
            {
                cSS->blockSignals(true);
                cSS->setCurrentIndex(newIndex);
                cSS->blockSignals(false);
            }
        }
        m_colorPresets->onColorSpaceSelected(newClrId);
        //uiCapture->m_colorSpaceSelectorButton->setText(colorSpace->name());
    });

    connect(m_mixer.data(), &EXColorMixState::sigColorModelChanged, uiCapture, [this, uiCapture, cSS](const ColorModelId newClrId) {

        ColorModelId oldClrId = uiCapture->m_colorSpaceSelector2->currentData().value<ColorModelId>();
        if (newClrId != oldClrId) {
            int newIndex = cSS->findData(newClrId);
            if (newIndex >= 0)
            {
                cSS->blockSignals(true);
                cSS->setCurrentIndex(newIndex);
                cSS->blockSignals(false);
            }
        }
        m_colorPresets->onColorSpaceSelected(newClrId);
        //uiCapture->m_colorSpaceSelectorButton->setText(colorSpace->name());
    });

    connect(m_mixer.data(), &EXColorMixState::sigCanvasReady, uiCapture, [this, uiCapture]() {
        Q_UNUSED(this);
        int activePreset = m_colorPresets->activePresetIndex();
        uiCapture->onNewPresetSelected(activePreset);
    });

    connect(this, &EXActionBus::sigInputPortsChanged, m_tmpui->m_midiPanel, &EXMIDIPanelWidget::onPortsAvailable);
    connect(this->m_tmpui->m_midiPanel->mappingTable, &MappingTableWidget::sigPortSelected, m_midiListener, [this](QString portName) {
        m_midiListener->startListeningTo(portName);
    });

    MappingTableWidget* mappingTableCapture = m_tmpui->m_midiPanel->mappingTable;
    connect(m_midiListener, &MidiListener::sigNowListeningTo, mappingTableCapture, &MappingTableWidget::onMidiDeviceChanged);


    connect(m_midiListener, &MidiListener::sigErrorOccurred, m_tmpui->m_midiPanel, &EXMIDIPanelWidget::onError);
    connect(m_midiListener, &MidiListener::sigMidiMessageArrived, logPanelCapture, [logPanelCapture](const MidiEvent& evt) {
        logPanelCapture->onMidiMessage(evt);
    });

    connect(this, &EXActionBus::sigLogMessage, logPanelCapture, [logPanelCapture](const QString& message) {
        logPanelCapture->appendLine(message);
    });

    connect(m_midiListener, &MidiListener::sigMidiMessageArrived, this, &EXActionBus::onMidiMessage);

    connect(this->m_tmpui->m_midiPanel->mappingTable, &MappingTableWidget::sigMappingsEdited, this, [this, mappingTableCapture]() {
        QVector<MappingEntry> NewMappings = mappingTableCapture->collectMappingsFromTable();
        this->m_mapper->setMappings(NewMappings);
        qDebug() << "EXMIDIMapperPresetControl: Updated mappings";
    });


    connect(this, &EXActionBus::sigKnobTurned, m_mixer, [this](int deviceIndex, int value) {
        float newWeight = (float)(value)/(float)(127);
        this->m_mixer->onIngredientColorWeightChanged(deviceIndex, newWeight);
    });

    //
    // connect(m_tmpui, &EXColorSelectorDock::sigMixFromColorsButtonPressed,
    //     this, [this]() {
    //         size_t activePresetN = m_colorPresets->m_activePreset;
    //         if (activePresetN>7) { qDebug() << "Error: activePreset out of range"; }
    //         else {
    //             auto activePreset = &m_colorPresets->m_colorMixPresets[activePresetN];
    //             activePreset->m_mixFromGradients = false;
    //         }
    //     }
    // );
    //
    // connect(m_tmpui, &EXColorSelectorDock::sigMixFromGradientsButtonPressed,
    //     this, [this]() {
    //         size_t activePresetN = m_colorPresets->m_activePreset;
    //         if (activePresetN>7) { qDebug() << "Error: activePreset out of range"; }
    //         else {
    //             auto activePreset = &m_colorPresets->m_colorMixPresets[activePresetN];
    //             activePreset->m_mixFromGradients = true;
    //         }
    //     }
    // );
    //
    // m_mixer->connectChannelPlane(m_ui->m_plane);
    // m_settingsState->connectChannelPlane(m_ui->m_plane);
    //
    // connect(m_mixer.data(), &EXColorMixState::sigColorModelChanged, m_ui->m_plane, [this]() {
    //     EXSettingsState::instance()->applySettingsToPlane(this->m_ui->m_plane);
    // });
    //
    //
    // connect(m_mixer.data(), &EXColorMixState::sigColorModelChanged, this, [this]() {
    //     this->m_ui->updateSliders();
    // });
    // connect(m_settingsState.data(), &EXSettingsState::sigSettingsChanged, this, [this]() {
    //     this->m_ui->updateSliders();
    // });


    //################################################################################
    //##  Initialization of variables
    //################################################################################

    this->currentPorts = m_midiListener->availableInputPorts();
    // Try to connect to first available port
    if (!currentPorts.isEmpty()) {
        this->currentPortName = currentPorts[0];
        qDebug() << "Initializing EXActionbus." << "Starting new receiver for:" << currentPortName;
        m_midiListener->startListeningTo(currentPortName);
        emit sigInputPortsChanged(currentPorts);
    } else {
        this->currentPortName = "";
    }
    m_tmpui->m_midiPanel->onPortSelected();
    //
    // this->portRefreshTimer = new QTimer(this);
    // connect(portRefreshTimer, &QTimer::timeout, this, &EXActionBus::onRefreshMidiPorts);
    // portRefreshTimer->start(2000);

    startSignalLogging();

    m_tmpui->m_midiPanel->loadSettings();
    qDebug() << "Loading initial Colors...";
    m_tmpui->loadColorsFromPreset(0);
}

void EXActionBus::onKritaBaseColorChanged(const QVector3D& newlyPickedColor) {

    qDebug() << "ActionBus fired onKritaBaseColorChanged";
    //Option A: No responsive UI element had been selected
    int selectedClrPatch = this->m_tmpui->selectedMixChannel();
    if (selectedClrPatch < 0) { return; }
    qDebug() << "ColorPatch selected:" << selectedClrPatch;
    // Option B a Clr Patch has been selected; active color preset will now be modified
    //TODO: make selection instead
    int activePreset = this->m_tmpui->selectedPreset();
    // m_colorPresets->m_activePreset = activePreset;
    m_colorPresets->onPresetSelected(activePreset);

    qDebug() << "Preset selected:" << activePreset;

    // m_colorPresets->m_colorMixPresets[activePreset].m_ingredientMixColors[selectedClrPatch] = newlyPickedColor;
    m_colorPresets->onMixColorChanged(selectedClrPatch, newlyPickedColor);
    m_tmpui->onNewPresetSelected(activePreset);
    //TODO: without informing EXChannelPlane, this whacks the color selector
    m_mixer->onColorPresetChanged(activePreset);
}

void EXActionBus::onRefreshMidiPorts() {
    QStringList newPorts = m_midiListener->availableInputPorts();

    if (newPorts != currentPorts) {
        currentPorts = newPorts;
        m_ui->m_midiPanel->onPortsAvailable(); // midiPanel fetches available ports from EXActionBus
    }

    const QString portName = m_ui->m_midiPanel->mappingTable->desiredPort();

    if (!portName.isEmpty()) {
        int index = currentPorts.indexOf(portName);
        if (index != -1) {
            if (portName != currentPortName) {
                qDebug() << "Refreshing Midi ports." << "Starting new receiver for:" << portName;
                m_midiListener->startListeningTo(portName);
                currentPortName = portName;
                m_ui->m_midiPanel->onPortSelected(); // midiPanel fetches currentPortName from EXActionBus
                emit sigLogMessage(QString("[Auto-connected to port %1]").arg(portName));
            }
        } else {
            emit sigLogMessage(QString("[Failed to connect to port %1: Not found]").arg(portName));
        }
    } else {
        if (!currentPortName.isEmpty()) {
            qDebug() << "Closing Midi Receiver";
            m_midiListener->stopListening();
            currentPortName.clear();
        }
        m_ui->m_midiPanel->mappingTable->setConnectionStatus(false);
    }
}

void EXActionBus::onPortSelected(const QString& portName)
{
    int index = currentPorts.indexOf(portName);
    if (index != -1) {
        // m_midiListener->openPort(index);
        qDebug() << "New port selected." << "Starting new receiver for:" << portName;
        m_midiListener->startListeningTo(portName);
        currentPortName = portName;
        //logPanel->appendLine(QString("[Connected to port %1]").arg(portName));
        emit sigLogMessage(QString("[Connected to port %1]").arg(portName));
        m_ui->m_midiPanel->mappingTable->setConnectionStatus(true);
    } else {
        emit sigLogMessage(QString("[Failed to connect to port %1: Not found]").arg(portName));
    }
}

void EXActionBus::onMidiMessage(const MidiEvent& evt)
{
    MappedMidiEvent mmEvt = m_mapper->mapMidiEvent(evt);
    // negative values indicate no action needs to be taken (e.g. button ist still being pressed)
    if (mmEvt.ignoreEvent || mmEvt.mappedAction == EXMappedMidiAction::None) {
        // qDebug() << "MidiEvent ignored:" << EXMappedMidiActionToString(mmEvt.mappedAction) << QString("(%1)").arg(mmEvt.value);
    } else {
        int deviceIndex = MidiActionDeviceIndex(mmEvt.mappedAction);
        if (MidiActionMapsToKnob(mmEvt.mappedAction)) {
            emit sigKnobTurned(deviceIndex, mmEvt.value);
        }
        else if (MidiActionMapsToPad(mmEvt.mappedAction)) {
            emit sigPadPressed(deviceIndex, mmEvt.value);
        }
    }
}


void EXActionBus::startSignalLogging() {

    EXColorSelectorDock* uiCapture = m_tmpui;
    QComboBox* presetSelector = uiCapture->m_presetSelector;
    QComboBox* cSS = uiCapture->m_colorSpaceSelector2;

    connect(m_mixer.data(), &EXColorMixState::sigKritaBaseColorChanged, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "Krita Base color changed";
    });

    connect(m_mixer.data(), &EXColorMixState::sigColorChanged, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "Krita mixed color changed";
    });

    connect(cSS, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "ColorSpaceSelector index changed to" << index;
    });

    connect(presetSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "PresetSelector index changed to" << index;
    });

    connect(uiCapture, &EXColorSelectorDock::sigColorPatchWidgetSelected, this, [this](int index) {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "ColorPatchWidgetSelected" << index;
    });

    //TODO: Change ColorModel when ColorSpace changes
    connect(m_mixer.data(), &EXColorMixState::sigColorSpaceChanged, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal" << "ColorSpaceChanged";
    });

    connect(m_mixer.data(), &EXColorMixState::sigColorModelChanged, this, [this](ColorModelId clrId) {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "ColormodelChanged" << clrId;
    });

    connect(m_settingsState.data(), &EXSettingsState::sigSettingsChanged, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "SettingsChanged";
    });

    connect(this, &EXActionBus::sigInputPortsChanged, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "sigInputPortsChanged";
    });

    connect(m_midiListener, &MidiListener::sigNowListeningTo, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "sigNowListeningTo";
    });

    connect(m_midiListener, &MidiListener::sigErrorOccurred, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "sigErrorOccurred";
    });
    connect(m_midiListener, &MidiListener::sigMidiMessageArrived, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "sigMidiMessageArrived";
    });

    connect(this, &EXActionBus::sigLogMessage, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "sigLogMessage";
    });

    connect(this->m_tmpui->m_midiPanel->mappingTable, &MappingTableWidget::sigMappingsEdited, this, [this]() {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "sigMappingsEdited";
    });

    connect(this, &EXActionBus::sigKnobTurned, this, [this](int knob, int value) {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "sigKnobTurned" << knob << "with value:" << value;
    });

    connect(this, &EXActionBus::sigPadPressed, this, [this](int knob, int value) {
        Q_UNUSED(this);
        qDebug() << "Signal:" << "sigPadPressed" << knob << "with value:" << value;
    });
}




