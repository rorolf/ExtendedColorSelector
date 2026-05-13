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

    for (int k = 0; k<9; ++k)
    {
        auto newChannelWidget = new EXColorPatchWidget();
        m_colorPatchWidgets[k] = newChannelWidget;

        if (k!=4)
        {
            connect(newChannelWidget, &EXColorPatchWidget::sigClicked,
                this, [this, k]() {
                    int curSelect = this->m_selectedColorPatchWidget;
                    if (curSelect<0) {
                        // app is free to focus on an object
                        this->m_selectedColorPatchWidget = k;
                        this->m_colorPatchWidgets[k]->m_selected = true;
                        this->m_colorPatchWidgets[k]->update();
                        emit sigColorPatchWidgetSelected(k);
                    } else if (curSelect != k) {
                        // app already has a focus; do nothing
                    } else {
                        // release focus from object
                        this->m_selectedColorPatchWidget = -1;
                        this->m_colorPatchWidgets[k]->m_selected = false;
                        this->m_colorPatchWidgets[k]->update();
                        emit sigColorPatchWidgetSelected(-1);
                    }
                }
            );
        }
        else
        {
            connect(EXColorMixState::instance(), &EXColorMixState::sigKritaBaseColorChanged,
                this, [this, k](QVector3D newClr) {
                    Q_UNUSED(this);
                    if (this->m_selectedColorPatchWidget < 0) {
                        QColor newQClr = EXColorMixState::instance()->toQColor(newClr);
                        this->m_colorPatchWidgets[k]->m_color = newQClr;
                        this->m_colorPatchWidgets[k]->update();
                        // newChannelWidget->onColorSelected(newQClr);
                    }
                }
            );
        }

        int x = k%3;
        int y = k/3;
        mixChannelLayout->addWidget(newChannelWidget, x, y);
    }

    QButtonGroup *mixerModeButtonGroup = new QButtonGroup(this);
    m_mixingModeSelector = mixerModeButtonGroup;

    QRadioButton *mixFromColorsButton = new QRadioButton(this);
    mixFromColorsButton->setText("Color Palette");
    mixFromColorsButton->setChecked(true);

    connect(mixFromColorsButton, &QRadioButton::clicked, this, &EXColorSelectorDock::sigMixFromColorsButtonPressed);

    QRadioButton *mixFromGradientsButton = new QRadioButton(this);
    mixFromGradientsButton->setText("Gradient Palette");
    mixFromColorsButton->setChecked(false);

    connect(mixFromGradientsButton, &QRadioButton::clicked, this, &EXColorSelectorDock::sigMixFromGradientsButtonPressed);

    mixerModeButtonGroup->addButton(mixFromColorsButton, 0);
    mixerModeButtonGroup->addButton(mixFromGradientsButton, 1);

    m_mixResultColorPatch = new EXColorPatchWidget();

    mixSideLayout->addWidget(mixFromColorsButton);
    mixSideLayout->addWidget(mixFromGradientsButton);
    mixSideLayout->addWidget(m_mixResultColorPatch);

    mixPresetLayout->addLayout(mixChannelLayout);
    mixPresetLayout->addLayout(mixSideLayout);

    colorMixLayout->addLayout(mixPresetLayout);

    m_midiPanel = new EXMIDIPanelWidget();
    QVBoxLayout* midiLayout = new QVBoxLayout();
    midiLayout->addWidget(m_midiPanel);
    m_tabWidget = new QTabWidget();

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
    //## Added TabWidget logic
    //################################################################################

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
    int clrPatch = this->m_selectedColorPatchWidget;
    return clrPatch - (int)(clrPatch > 4);
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

    qDebug() << "Loading Colors from Preset" << activePreset;
    for (size_t k=0; k<m_colorPatchWidgets.size(); ++k) {
        if (k == 4) continue;
        int k2 = k - (size_t)(k>4);
        qDebug() << "Loading Color" << k2 << "into ColorPatchWidget" << k;
        QVector3D ingClr = newPreset.m_ingredientMixColors[k2];
        qDebug() << "Converting (" << ingClr[0] << ingClr[1] << ingClr[2] << ") to QColor...";
        QColor newClr = EXColorMixState::instance()->toQColor(ingClr);
        // float clrX = newPreset->m_ingredientMixColors[k2][0];
        // float clrY = newPreset->m_ingredientMixColors[k2][1];
        // float clrZ = newPreset->m_ingredientMixColors[k2][2];
        // qDebug() << "Loaded new Color for ColorPatch" << k << "from color" << k2 << QString("(%1,%2,%3)").arg(clrX).arg(clrY).arg(clrZ);
        // m_colorPatchWidgets[k]->onColorSelected(newClr);
        m_colorPatchWidgets[k]->m_color = newClr;
        m_colorPatchWidgets[k]->update();
    }
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
