#include <QVBoxLayout>

#include <KisViewManager.h>
#include <KoColorDisplayRendererInterface.h>
#include <kis_canvas_resource_provider.h>
#include <kis_display_color_converter.h>
#include <kis_icon_utils.h>

#include "EXColorModel.h"
#include "EXColorSelectorDock.h"
#include "EXActionbus.h"

EXColorSelectorDock::EXColorSelectorDock()
    : QDockWidget("Extended Color Selector")
    , m_canvas(nullptr)
    , m_colorMixState(EXColorMixState::instance())
    , m_settingsState(EXSettingsState::instance())
{
    this->setObjectName("EXColorMixerDock");
    m_canvas = nullptr;

    auto mainLayout = new QVBoxLayout();
    mainLayout->setObjectName("MainLayout");

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

    auto colorSpaceSelectorLabel = new QLabel("ColorSpace:");

    m_colorSpaceSelector2 = new QComboBox(this);
    m_colorSpaceSelector2->setEditable(false);

    for (auto clrid : { ColorModelId::LinearRgb, ColorModelId::Srgb,
                        ColorModelId::Xyz,
                        ColorModelId::Lab, ColorModelId::Lch, ColorModelId::Oklab, ColorModelId::Oklch})
    {
        QString modelName = EXColorModel::modelNameFromId(clrid);
        m_colorSpaceSelector2->addItem(modelName, QVariant(clrid));
    }

    presetSpaceLayout->addWidget(m_presetSelector);
    presetSpaceLayout->addWidget(colorSpaceSelectorLabel);
    presetSpaceLayout->addWidget(m_colorSpaceSelector2);
    mainLayout->addLayout(presetSpaceLayout);

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
                    } else if (curSelect != k) {
                        // app already has a focus; do nothing
                    } else {
                        // release focus from object
                        this->m_selectedColorPatchWidget = -1;
                        this->m_colorPatchWidgets[k]->m_selected = false;
                        this->m_colorPatchWidgets[k]->update();
                    }
                }
            );
        }
        else
        {
            connect(EXColorMixState::instance(), &EXColorMixState::sigKritaBaseColorChanged,
                this, [this, k](QVector3D newClr) {
                    Q_UNUSED(this);
                    QColor newQClr = EXColorMixState::instance()->toQColor(newClr);
                    this->m_colorPatchWidgets[k]->m_color = newQClr;
                    this->m_colorPatchWidgets[k]->update();
                    // newChannelWidget->onColorSelected(newQClr);
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

    connect(EXColorMixState::instance(), &EXColorMixState::sigColorChanged,
        this, [this](QVector3D newClrV) {
            Q_UNUSED(this);
            auto newClr = EXColorMixState::instance()->toQColor(newClrV);
            this->m_mixResultColorPatch->m_color = newClr;
            this->m_mixResultColorPatch->update();
        }
    );

    mixSideLayout->addWidget(mixFromColorsButton);
    mixSideLayout->addWidget(mixFromGradientsButton);
    mixSideLayout->addWidget(m_mixResultColorPatch);

    mixPresetLayout->addLayout(mixChannelLayout);
    mixPresetLayout->addLayout(mixSideLayout);

    mainLayout->addLayout(mixPresetLayout);


    //################################################################################
    //## original code
    //################################################################################


    auto colorSpaceLayout = new QHBoxLayout(this);
    m_colorSpaceSelectorButton = new KisPopupButton(this);
    m_colorSpaceSelector = new KisColorSpaceSelector(this);
    m_colorSpaceSelector->showColorBrowserButton(false);
    m_useLayerColorSpaceButton = new QPushButton(this);
    m_useLayerColorSpaceButton->setCheckable(true);
    m_colorSpaceSelectorButton->setPopupWidget(m_colorSpaceSelector);
    m_colorSpaceSelectorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    colorSpaceLayout->addWidget(m_colorSpaceSelectorButton);
    colorSpaceLayout->addWidget(m_useLayerColorSpaceButton);
    mainLayout->addLayout(colorSpaceLayout);

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

    m_plane = new EXChannelPlane(this);
    m_plane->setColorModel(ColorModelFactory::fromId((ColorModelId)m_settingsState->globalSettings.currentColorModel));
    m_colorMixState->connectChannelPlane(m_plane);
    m_settingsState->connectChannelPlane(m_plane);
    m_colorPatchPopup->connectToWidget(m_plane);

    m_sliders = new EXChannelSlidersGroup(QVector<ColorModelId>(), this);
    m_colorModelSwitchers = new EXColorModelSwitchers(m_colorMixState, m_settingsState, this);

    mainLayout->addWidget(m_plane);
    mainLayout->addWidget(m_colorModelSwitchers);
    mainLayout->addWidget(m_sliders);
    mainLayout->addStretch(1);

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
    mainLayout->addLayout(settingsButtonLayout);

    auto mainWidget = new QWidget(this);
    mainWidget->setLayout(mainLayout);
    setWidget(mainWidget);

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

    connect(m_colorMixState.data(), &EXColorMixState::sigColorModelChanged, m_plane, [this]() {
        m_settingsState->applySettingsToPlane(m_plane);
    });

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
