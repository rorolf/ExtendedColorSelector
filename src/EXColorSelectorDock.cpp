#include <QVBoxLayout>

#include <KisViewManager.h>
#include <KoColorDisplayRendererInterface.h>
#include <kis_canvas_resource_provider.h>
#include <kis_display_color_converter.h>
#include <kis_icon_utils.h>
#include <qtabwidget.h>

#include "EXColorMixState.h"
#include "EXColorModel.h"
#include "EXColorSelectorDock.h"
#include "EXActionbus.h"
#include "EXColorPresetStore.h"
#include "EXMIDIPanelWidget.h"

EXColorSelectorDock::EXColorSelectorDock()
    : QDockWidget("Extended Color Selector")
    , m_canvas(nullptr)
    , m_colorMixState(EXColorMixState::instance())
    , m_settingsState(EXSettingsState::instance())
{
    this->setObjectName("EXColorMixerDock");
    m_canvas = nullptr;

    auto colorMixLayout = new QVBoxLayout();
    colorMixLayout->setObjectName("ColorMixLayout");

    m_colorPatchPopup = new EXColorPatchPopup(this);
    // connect(m_colorMixState.data(), &EXColorMixState::sigColorChanged, this, [this]() {
    //     m_colorPatchPopup->updateColor(m_colorMixState->qColor());
    // });

    //################################################################################
    //## new code
    //################################################################################

    auto presetSpaceLayout = new QHBoxLayout();
    m_presetSelector = new QComboBox(this);
    m_presetSelector->setEditable(false);
    for (int k=0; k<8;++k)
    {
        m_presetSelector->addItem("Preset " + QString::number(k+1), QVariant::fromValue(k));
    }

    auto colorModelSelectorLabel = new QLabel("ColorSpace/Model:");

    auto colorSelectorButtonLayout = new QVBoxLayout();

    auto colorSpaceLayout = new QHBoxLayout(this);
    m_colorSpaceSelectorButton = new KisPopupButton(this);
    m_colorSpaceSelector = new KisColorSpaceSelector(this);
    m_colorSpaceSelector->showColorBrowserButton(false);
    m_useLayerColorSpaceButton = new QPushButton(this);
    m_useLayerColorSpaceButton->setCheckable(true);
    m_colorSpaceSelectorButton->setPopupWidget(m_colorSpaceSelector);
    m_colorSpaceSelectorButton->setMaximumWidth(360);
    m_useLayerColorSpaceButton->setMaximumSize(40, 40);
    m_colorSpaceSelectorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    colorSpaceLayout->addWidget(m_colorSpaceSelectorButton);
    colorSpaceLayout->addWidget(m_useLayerColorSpaceButton);
    colorSelectorButtonLayout->addLayout(colorSpaceLayout);

    m_colorModelSelector = new QComboBox(this);
    m_colorModelSelector->setEditable(false);

    for (auto clrid : ColorModelFactory::AllModels)
    {
        QString modelName = EXColorModel::modelNameFromId(clrid);
        m_colorModelSelector->addItem(modelName, clrid);
    }

    colorSelectorButtonLayout->addWidget(m_colorModelSelector);

    presetSpaceLayout->addWidget(m_presetSelector);
    presetSpaceLayout->addWidget(colorModelSelectorLabel);
    presetSpaceLayout->addLayout(colorSelectorButtonLayout);
    colorMixLayout->addLayout(presetSpaceLayout);

    // ###########################################################

    auto mixPresetLayout = new QHBoxLayout();
    mixPresetLayout->setObjectName("mixPresetLayout");
    auto mixChannelLayout = new QGridLayout();
    mixChannelLayout->setObjectName("mixChannelLayout");
    auto mixSideLayout = new QVBoxLayout();
    mixSideLayout->setObjectName("mixSideLayout");

    m_kritaBaseColorPatchWidget = new EXColorPatchWidget();
    connect(EXColorMixState::instance(), &EXColorMixState::sigKritaBaseColorChanged,
        this, [this](QVector3D newClr) {
            Q_UNUSED(this);
            if (this->m_selectedColorPatchWidget < 0) {
                QColor newQClr = EXColorMixState::instance()->toQColor(newClr);
                this->m_kritaBaseColorPatchWidget->m_color = newQClr;
                this->m_kritaBaseColorPatchWidget->update();
                // newChannelWidget->onColorSelected(newQClr);
            }
        }
    );

    for (size_t k = 0; k<this->m_colorPatchWidgets.size(); ++k) {
        auto newChannelWidget = new EXColorPatchWidget();
        m_colorPatchWidgets[k] = newChannelWidget;

        connect(newChannelWidget, &EXColorPatchWidget::sigClicked,
            this, [this, k]() {
                int curSelect = this->m_selectedColorPatchWidget;
                if (curSelect<0) {
                    // app is free to focus on an object
                    this->m_selectedColorPatchWidget = k;
                    this->m_colorPatchWidgets[k]->m_selected = true;
                    this->m_colorPatchWidgets[k]->update();

                    for (auto rButton : m_mixingModeSelector->buttons()) {
                        rButton->setDisabled(false);
                    }

                    emit sigColorPatchWidgetSelected(k);
                } else if (curSelect != (int)k) {
                    // app already has a focus; do nothing
                } else {
                    // release focus from object
                    this->m_selectedColorPatchWidget = -1;
                    this->m_colorPatchWidgets[k]->m_selected = false;
                    this->m_colorPatchWidgets[k]->update();

                    for (auto rButton : m_mixingModeSelector->buttons()) {
                        rButton->setDisabled(true);
                    }

                    emit sigColorPatchWidgetSelected(-1);
                }
            }
        );
        if (k==4) {
            // in the middle of a 3x3 grid
            mixChannelLayout->addWidget(m_kritaBaseColorPatchWidget, 1, 1);
        }
        int shift = (k>3);
        int x = (k+shift)%3;
        int y = (k+shift)/3;
        mixChannelLayout->addWidget(newChannelWidget, x, y);
    }

    QButtonGroup *mixerModeButtonGroup = new QButtonGroup(this);
    m_mixingModeSelector = mixerModeButtonGroup;

    m_mixFromRawColorButton = new QRadioButton(this);
    m_mixFromRawColorButton->setText("Raw Color Channel");
    m_mixFromRawColorButton->setChecked(true);
    m_mixFromRawColorButton->setDisabled(true);

    connect(m_mixFromRawColorButton, &QRadioButton::clicked, this, &EXColorSelectorDock::sigMixFromColorsButtonPressed);

    m_mixFromGradientButton = new QRadioButton(this);
    m_mixFromGradientButton->setText("Gradient Color Channel");
    m_mixFromGradientButton->setChecked(false);
    m_mixFromGradientButton->setDisabled(true);

    connect(m_mixFromGradientButton, &QRadioButton::clicked, this, &EXColorSelectorDock::sigMixFromGradientsButtonPressed);

    mixerModeButtonGroup->addButton(m_mixFromRawColorButton, 0);
    mixerModeButtonGroup->addButton(m_mixFromGradientButton, 1);

    m_mixResultColorPatch = new EXColorPatchWidget();

    mixSideLayout->addWidget(m_mixFromRawColorButton);
    mixSideLayout->addWidget(m_mixFromGradientButton);
    mixSideLayout->addWidget(m_mixResultColorPatch);

    mixPresetLayout->addLayout(mixChannelLayout);
    mixPresetLayout->addLayout(mixSideLayout);

    colorMixLayout->addLayout(mixPresetLayout);

    m_midiPanel = new EXMIDIPanelWidget();
    QVBoxLayout* midiLayout = new QVBoxLayout();
    midiLayout->addWidget(m_midiPanel);

    //################################################################################
    //## original code
    //################################################################################

    connect(m_colorMixState.data(), &EXColorMixState::sigColorSpaceChanged, this, [this](const KoColorSpace *colorSpace) {
        if (colorSpace != m_colorSpaceSelector->currentColorSpace()) {
            m_colorSpaceSelector->setCurrentColorSpace(colorSpace);
        }
        m_colorSpaceSelectorButton->setText(colorSpace->name());
    });
    connect(m_useLayerColorSpaceButton, &QPushButton::toggled, this, [this](bool checked) {
        auto &settings = m_settingsState->globalSettings;
        settings.useLayerColorSpace = checked;
        m_colorMixState->setUseLayerColorSpace(checked);
        m_colorSpaceSelectorButton->setEnabled(!checked);
        settings.customColorSpace = m_colorSpaceSelector->currentColorSpace();
        m_useLayerColorSpaceButton->setIcon(checked ? KisIconUtils::loadIcon("chain-icon")
                                                    : KisIconUtils::loadIcon("chain-broken-icon"));
        settings.writeAll();
    });
    connect(m_colorSpaceSelector,
            SIGNAL(colorSpaceChanged(const KoColorSpace *)),
            this,
            SLOT(onColorSpaceSelected(const KoColorSpace *)));

    //################################################################################
    //## EXGradientWidget
    //################################################################################

    m_gradientWidget = EXGradientWidget::fromPreset(this, EXColorPresetStore::instance()->activePreset(), 0);
    colorMixLayout->addWidget(m_gradientWidget);

    //################################################################################
    //## EXChannelPlane inside a KisPopupButton
    //################################################################################

    auto colorSelectLayout = new QVBoxLayout();

    m_plane = new EXChannelPlane(this);
    m_plane->setColorModel(ColorModelFactory::fromId((ColorModelId)m_settingsState->globalSettings.currentColorModel));
    m_colorMixState->connectChannelPlane(m_plane);
    m_settingsState->connectChannelPlane(m_plane);
    m_colorPatchPopup->connectToWidget(m_plane);

    m_sliders = new EXChannelSlidersGroup(QVector<ColorModelId>(), this);
    m_colorModelSwitchers = new EXColorModelSwitchers(m_colorMixState, m_settingsState, this);

    colorSelectLayout->addWidget(m_plane);
    colorSelectLayout->addWidget(m_colorModelSwitchers);
    colorSelectLayout->addWidget(m_sliders);
    colorSelectLayout->addStretch(1);

    m_settings = new EXPerColorModelSettingsDialog(m_settingsState, this);
    m_globalSettings = new EXGlobalSettingsDialog(m_settingsState, this);

    auto settingsButtonLayout = new QHBoxLayout(this);
    auto settingsButton = new QPushButton();
    settingsButton->setIcon(KisIconUtils::loadIcon(("configure")));
    settingsButton->setFlat(true);
    connect(settingsButton, &QPushButton::clicked, this, [this]() {
        m_settings->exec();
    });
    auto globalSettingsButton = new QPushButton(this);
    globalSettingsButton->setIcon(KisIconUtils::loadIcon(("applications-system")));
    globalSettingsButton->setFlat(true);
    connect(globalSettingsButton, &QPushButton::clicked, this, [this]() {
        m_globalSettings->exec();
    });
    settingsButtonLayout->addWidget(settingsButton);
    settingsButtonLayout->addStretch(1);
    settingsButtonLayout->addWidget(globalSettingsButton);
    colorSelectLayout->addLayout(settingsButtonLayout);

    auto colorSelectWidget = new QWidget(this);
    colorSelectWidget->setMaximumSize(800, 600);
    colorSelectWidget->setLayout(colorSelectLayout);

    m_colorSelectorPupupButton = new KisPopupButton(this);
    m_colorSelectorPupupButton->setText("Select Color");
    m_colorSelectorPupupButton->setPopupWidget(colorSelectWidget);
    m_colorSelectorPupupButton->setMinimumWidth(200);
    m_colorSelectorPupupButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    colorMixLayout->addWidget(m_colorSelectorPupupButton);

    auto colorMixWidget = new QWidget(this);
    colorMixWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    colorMixWidget->setLayout(colorMixLayout);



    //################################################################################
    //## TabWidget logic
    //################################################################################

    m_tabWidget = new QTabWidget();
    m_tabWidget->addTab(colorMixWidget, "ColorSelector");
    m_tabWidget->addTab(m_midiPanel, "Midi");

    setWidget(m_tabWidget);

    m_portableSelector = new EXPortableColorSelector();

    m_useLayerColorSpaceButton->setChecked(m_settingsState->globalSettings.useLayerColorSpace);
    m_colorMixState->setUseLayerColorSpace(m_settingsState->globalSettings.useLayerColorSpace);
    if (m_settingsState->globalSettings.useLayerColorSpace) {
        m_useLayerColorSpaceButton->setIcon(KisIconUtils::loadIcon("chain-icon"));
    } else {
        m_useLayerColorSpaceButton->setIcon(KisIconUtils::loadIcon("chain-broken-icon"));
        auto customColorSpace = m_settingsState->globalSettings.customColorSpace;
        m_colorSpaceSelector->setCurrentColorSpace(customColorSpace);
        m_colorMixState->setColorSpace(customColorSpace);
    }

    updateSliders();
    connect(m_colorMixState.data(), &EXColorMixState::sigColorModelChanged, this, &EXColorSelectorDock::updateSliders);
    connect(m_settingsState.data(), &EXSettingsState::sigSettingsChanged, this, &EXColorSelectorDock::updateSliders);


    //################################################################################
    //## EXActionBus
    //################################################################################

    m_actionBus = EXActionBus::instance();
    m_actionBus->initializeAndConnectToEXS(this);
}

void EXColorSelectorDock::setViewManager(KisViewManager *kisview)
{
    m_portableSelector->setViewManager(kisview);
}

void EXColorSelectorDock::setCanvas(KoCanvasBase *canvas)
{
    m_canvas = qobject_cast<KisCanvas2 *>(canvas);
    if (m_canvas) {
        m_plane->setCanvas(m_canvas);
        m_sliders->setCanvas(m_canvas);
        m_portableSelector->setCanvas(m_canvas);
        m_colorMixState->setCanvas(m_canvas);
        Q_EMIT m_settingsState->sigSettingsChanged();
    }
}

void EXColorSelectorDock::unsetCanvas()
{
    m_canvas = nullptr;
    m_plane->setCanvas(nullptr);
    m_sliders->setCanvas(nullptr);
    m_colorMixState->setCanvas(nullptr);
    m_portableSelector->setCanvas(nullptr);
}


int EXColorSelectorDock::selectedPreset() const {
    // TODO: report QString instead
    return this->m_presetSelector->currentIndex();
}

int EXColorSelectorDock::selectedMixChannel() const {
    return this->m_selectedColorPatchWidget;
}

int EXColorSelectorDock::selectedGradientPoint() const {
    bool usesGradient = EXColorPresetStore::instance()->activePresetUsesGradient(this->selectedMixChannel());
    if (usesGradient) { return this->m_gradientWidget->selectedGradientPoint(); }
    else { return -1; }
}


void EXColorSelectorDock::enterEvent(QEvent *event)
{
    QDockWidget::enterEvent(event);
    m_colorPatchPopup->recordColor(m_colorMixState->qColor());
}

void EXColorSelectorDock::leaveEvent(QEvent *event)
{
    QDockWidget::leaveEvent(event);
    m_colorPatchPopup->hide();
}

void EXColorSelectorDock::onColorSpaceSelected(const KoColorSpace *colorSpace)
{
    auto &settings = m_settingsState->globalSettings;
    if (!settings.useLayerColorSpace) {
        m_colorMixState->setColorSpace(colorSpace);
        settings.customColorSpace = colorSpace;
        settings.writeAll();
    }
}

void EXColorSelectorDock::onNewPresetSelected(int activePreset) {
    int currentlySelectedPreset = this->m_presetSelector->currentIndex();
    if (activePreset != currentlySelectedPreset) {
        this->m_presetSelector->blockSignals(true);
        this->m_presetSelector->setCurrentIndex(activePreset);
        this->m_presetSelector->blockSignals(false);
    }
    this->loadColorsFromPreset(activePreset);
}

void EXColorSelectorDock::loadColorsFromPreset(int activePreset) {
    EXColorPreset newPreset = EXColorPresetStore::instance()->activePreset();

    for (size_t k=0; k<m_colorPatchWidgets.size(); ++k) {
        if (newPreset.m_useGradients[k]) {
            m_colorPatchWidgets[k]->loadColorFromGradient(newPreset.m_mixGradients[k]);
        } else {
            QVector3D ingClr = newPreset.m_ingredientMixColors[k];
            QColor newClr = m_colorMixState->toQColor(ingClr);
            m_colorPatchWidgets[k]->loadColorFromRaw(newClr);
        }
    }
    int mixChannel = this->selectedMixChannel();
    m_gradientWidget->usePreset(newPreset, mixChannel);
}

void EXColorSelectorDock::updateSliders()
{
    auto &settings = m_settingsState->settings[m_colorMixState->colorModel()->id()];
    if (settings.slidersEnabled) {
        auto sliders = QVector(settings.extraSliders);
        sliders.prepend(m_colorMixState->colorModel()->id());
        m_sliders->resetColorModels(sliders);
    } else {
        m_sliders->resetColorModels(settings.extraSliders);
    }

    for (auto sliders : m_sliders->sliders()) {
        for (auto slider : sliders->sliders()) {
            m_colorMixState->connectChannelSlider(slider);
            m_settingsState->connectChannelSlider(slider);
            m_colorPatchPopup->connectToWidget(slider->bar());
        }
    }
}
