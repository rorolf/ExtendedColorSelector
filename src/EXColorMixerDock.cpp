

#include <QVBoxLayout>

#include <KisViewManager.h>
#include <KoColorDisplayRendererInterface.h>
#include <kis_canvas_resource_provider.h>
#include <kis_display_color_converter.h>
#include <kis_icon_utils.h>
#include <qboxlayout.h>
#include <QGridLayout>
#include <qbuttongroup.h>
#include <qcombobox.h>
#include <QLabel>
#include <qradiobutton.h>
#include <qvector2d.h>
#include <qvector3d.h>
#include <QButtonGroup>
#include <QRadioButton>

#include "EXActionbus.h"
#include "EXColorMixState.h"
#include "EXColorPresetStore.h"
#include "EXColorModel.h"
#include "EXColorPatchWidget.h"
#include "EXSettings.h"
#include "EXSettingsState.h"
#include "kis_shared_ptr.h"
#include "EXColorMixerDock.h"

EXColorMixerDock::EXColorMixerDock()
    : QDockWidget("Extended Color Selector")
    , m_canvas(nullptr)
    , m_plane(nullptr)
    , m_sliders(nullptr)
    , m_presetSelector(nullptr)
    , m_colorSpaceSelector(nullptr)
    , m_selectedColorPatchWidget(-1)
    , m_colorPatchWidgets({})
    , m_mixingModeSelector(nullptr)
    , m_colorModelSwitchers(nullptr)
    , m_portableSelector(nullptr)
    , m_colorPatchPopup(nullptr)
    , m_globalSettings(nullptr)
    , m_settings(nullptr)
    //, m_colorMixState(EXColorMixState::instance())
    // , m_colorState(EXColorState::instance())
    // , m_colorPresets(EXColorPresetStore::instance())
    // , m_settingsState(EXSettingsState::instance())
    , m_actionBus(nullptr)
    // , m_colorSpaceSelectorButton(nullptr)
    // , m_useLayerColorSpaceButton(nullptr)
{
    this->setObjectName("EXColorMixerDock");

    auto mainLayout = new QVBoxLayout();
    mainLayout->setObjectName("MainLayout");

    m_colorPatchPopup = new EXColorPatchPopup(this);
    // connect(m_colorMixState.data(), &EXColorMixState::sigColorChanged, this, [this]() {
    //     m_colorPatchPopup->updateColor(m_colorState->qColor());
    // });

    auto presetSpaceLayout = new QHBoxLayout();
    m_presetSelector = new QComboBox(this);
    for (int k=0; k<8;++k)
    {
        m_presetSelector->addItem("Preset " + QString::number(k), QVariant::fromValue(k));
    }

    auto colorSpaceSelectorLabel = new QLabel("ColorSpace:");

    m_colorSpaceSelector = new QComboBox(this);
    m_colorSpaceSelector->setEditable(false);

    for (auto clrid : { ColorModelId::LinearRgb, ColorModelId::Srgb,
                        ColorModelId::Xyz,
                        ColorModelId::Lab, ColorModelId::Lch, ColorModelId::Oklab, ColorModelId::Oklch})
    {
        QString modelName = EXColorModel::modelNameFromId(clrid);
        m_colorSpaceSelector->addItem(modelName, QVariant(clrid));
    }
    // connect(
    //     m_colorSpaceSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
    //     this, [this](int newIndex) {
    //         auto data = m_colorSpaceSelector->itemData(newIndex);
    //         if (data.isValid())
    //         {
    //             ColorModelId newClrId = static_cast<ColorModelId>(data.value<int>());
    //             m_colorMixState->setColorModel(newClrId);
    //         }
    //     }
    // );

    // connect(m_colorState.data(), &EXColorState::sigColorSpaceChanged, this, [this](const KoColorSpace *colorSpace) {
    //
    //     auto newColorModel = ColorModelFactory::fromKoColorSpace(colorSpace);
    //     ColorModelId newClrId = newColorModel->id();
    //     delete newColorModel;
    //     ColorModelId oldClrId = static_cast<ColorModelId>(m_colorSpaceSelector->currentData().value<int>());
    //     if (newClrId != oldClrId) {
    //         int newIndex = m_colorSpaceSelector->findData(newClrId);
    //         if (newIndex >= 0)
    //         {
    //             m_colorSpaceSelector->blockSignals(true);
    //             m_colorSpaceSelector->setCurrentIndex(newIndex);
    //             m_colorSpaceSelector->blockSignals(false);
    //         }
    //     }
    //     //m_colorSpaceSelectorButton->setText(colorSpace->name());
    // });

    //m_colorSpaceSelectorButton->setPopupWidget(m_colorSpaceSelector);
    //m_colorSpaceSelectorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    presetSpaceLayout->addWidget(m_presetSelector);
    presetSpaceLayout->addWidget(colorSpaceSelectorLabel);
    presetSpaceLayout->addWidget(m_colorSpaceSelector);
    mainLayout->addLayout(presetSpaceLayout);

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
                this, [this, newChannelWidget](QVector3D newClr) {
                    newChannelWidget->m_color = EXColorMixState::instance()->toQColor(newClr);
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
    mixFromColorsButton->setWhatsThis("Color Palette");
    mixFromColorsButton->setChecked(true);

    // connect(mixFromColorsButton, &QRadioButton::clicked,
    //     this, [this]() {
    //         EXActionBus* actionBus = this->m_actionBus.data();
    //         if (actionBus) {
    //             size_t activePresetN = actionBus->m_colorPresets->m_activePreset;
    //             auto activePreset = &actionBus->m_colorPresets->m_colorMixPresets[activePresetN];
    //             activePreset->m_mixFromGradients = false;
    //         }
    //     }
    // );

    connect(mixFromColorsButton, &QRadioButton::clicked, this, &EXColorMixerDock::sigMixFromColorsButtonPressed);

    QRadioButton *mixFromGradientsButton = new QRadioButton(this);
    mixFromGradientsButton->setWhatsThis("Gradient Palette");
    mixFromColorsButton->setChecked(false);
    // connect(mixFromGradientsButton, &QRadioButton::clicked,
    //     this, [this]() {
    //         size_t activePresetN = m_colorPresets->m_activePreset;
    //         auto activePreset = &m_colorPresets->m_colorMixPresets[activePresetN];
    //         activePreset->m_mixFromGradients = true;
    //     }
    // );

    connect(mixFromGradientsButton, &QRadioButton::clicked, this, &EXColorMixerDock::sigMixFromGradientsButtonPressed);

    mixerModeButtonGroup->addButton(mixFromColorsButton, 0);
    mixerModeButtonGroup->addButton(mixFromGradientsButton, 1);

    auto mixResultColorPatch = new EXColorPatchWidget();

    connect(EXColorMixState::instance(), &EXColorMixState::sigColorChanged,
        this, [this, mixResultColorPatch](QVector3D newClrV) {
            auto newClr = EXColorMixState::instance()->toQColor(newClrV);
            mixResultColorPatch->onColorSelected(newClr);
        }
    );

    mixSideLayout->addWidget(mixFromColorsButton);
    mixSideLayout->addWidget(mixFromGradientsButton);
    mixSideLayout->addWidget(mixResultColorPatch);

    mixPresetLayout->addLayout(mixChannelLayout);
    mixPresetLayout->addLayout(mixSideLayout);

    mainLayout->addLayout(mixPresetLayout);


    // m_colorSpaceSelectorButton = new KisPopupButton(this);
    // m_colorSpaceSelector = new KisColorSpaceSelector(this);
    // m_colorSpaceSelector->showColorBrowserButton(false);
    // m_useLayerColorSpaceButton = new QPushButton(this);
    // m_useLayerColorSpaceButton->setCheckable(true);
    // m_colorSpaceSelectorButton->setPopupWidget(m_colorSpaceSelector);
    // m_colorSpaceSelectorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // colorSpaceLayout->addWidget(m_colorSpaceSelectorButton);
    // colorSpaceLayout->addWidget(m_useLayerColorSpaceButton);
    // mainLayout->addLayout(colorSpaceLayout);

    // connect(m_colorState.data(), &EXColorState::sigColorSpaceChanged, this, [this](const KoColorSpace *colorSpace) {
    //     if (colorSpace != m_colorSpaceSelector->currentColorSpace()) {
    //         m_colorSpaceSelector->setCurrentColorSpace(colorSpace);
    //     }
    //     m_colorSpaceSelectorButton->setText(colorSpace->name());
    // });
    // connect(m_useLayerColorSpaceButton, &QPushButton::toggled, this, [this](bool checked) {
    //     auto &settings = m_settingsState->globalSettings;
    //     settings.useLayerColorSpace = checked;
    //     m_colorState->setUseLayerColorSpace(checked);
    //     m_colorSpaceSelectorButton->setEnabled(!checked);
    //     settings.customColorSpace = m_colorSpaceSelector->currentColorSpace();
    //     m_useLayerColorSpaceButton->setIcon(checked ? KisIconUtils::loadIcon("chain-icon")
    //                                                 : KisIconUtils::loadIcon("chain-broken-icon"));
    //     settings.writeAll();
    // });
    // connect(m_colorSpaceSelector,
    //         SIGNAL(colorSpaceChanged(const KoColorSpace *)),
    //         this,
    //         SLOT(onColorSpaceSelected(const KoColorSpace *))
    // );

    m_plane = new EXChannelPlane(this);
    m_plane->setColorModel(ColorModelFactory::fromId((ColorModelId)EXSettingsState::instance()->globalSettings.currentColorModel));
    // m_colorState->connectChannelPlane(m_plane);
    // m_settingsState->connectChannelPlane(m_plane);
    m_colorPatchPopup->connectToWidget(m_plane);

    m_sliders = new EXChannelSlidersGroup(QVector<ColorModelId>(), this);
    m_colorModelSwitchers = new EXColorModelSwitchers(EXColorMixState::instance(), EXSettingsState::instance(), this);

    mainLayout->addWidget(m_plane);
    mainLayout->addWidget(m_colorModelSwitchers);
    mainLayout->addWidget(m_sliders);
    mainLayout->addStretch(1);

    m_settings = new EXPerColorModelSettingsDialog(EXSettingsState::instance(), this);
    m_globalSettings = new EXGlobalSettingsDialog(EXSettingsState::instance(), this);

    auto settingsButtonLayout = new QHBoxLayout();
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

    // m_useLayerColorSpaceButton->setChecked(m_settingsState->globalSettings.useLayerColorSpace);
    // m_colorState->setUseLayerColorSpace(m_settingsState->globalSettings.useLayerColorSpace);
    // if (m_settingsState->globalSettings.useLayerColorSpace) {
    //     m_useLayerColorSpaceButton->setIcon(KisIconUtils::loadIcon("chain-icon"));
    // } else {
    //     m_useLayerColorSpaceButton->setIcon(KisIconUtils::loadIcon("chain-broken-icon"));
    //     auto customColorSpace = m_settingsState->globalSettings.customColorSpace;
    //     m_colorSpaceSelector->setCurrentColorSpace(customColorSpace);
    //     m_colorState->setColorSpace(customColorSpace);
    // }

    // connect(m_colorState.data(), &EXColorState::sigColorModelChanged, m_plane, [this]() {
    //     EXSettingsState::instance()->applySettingsToPlane(m_plane);
    // });

    updateSliders();
    // connect(m_colorState.data(), &EXColorState::sigColorModelChanged, this, &EXColorMixerDock::updateSliders);
    // connect(m_settingsState.data(), &EXSettingsState::sigSettingsChanged, this, &EXColorMixerDock::updateSliders);


    // m_actionBus =  EXActionBus::instance(); //new EXActionBus(nullptr);
    // m_actionBus->initializeAndConnectTo(this);
}

void EXColorMixerDock::setViewManager(KisViewManager *kisview)
{
    m_portableSelector->setViewManager(kisview);
}

void EXColorMixerDock::setCanvas(KoCanvasBase *canvas)
{
    m_canvas = qobject_cast<KisCanvas2 *>(canvas);
    if (m_canvas) {
        m_plane->setCanvas(m_canvas);
        m_sliders->setCanvas(m_canvas);
        m_portableSelector->setCanvas(m_canvas);
        EXColorMixState::instance()->setCanvas(m_canvas);
        // TODO: erase code smell; apply distribution of responsibility
        Q_EMIT EXSettingsState::instance()->sigSettingsChanged();
    }
}

void EXColorMixerDock::unsetCanvas()
{
    m_canvas = nullptr;
    m_plane->setCanvas(nullptr);
    m_sliders->setCanvas(nullptr);
    EXColorMixState::instance()->setCanvas(nullptr);
    m_portableSelector->setCanvas(nullptr);
}

void EXColorMixerDock::enterEvent(QEvent *event)
{
    QDockWidget::enterEvent(event);
    m_colorPatchPopup->recordColor(EXColorMixState::instance()->qColor());
}

void EXColorMixerDock::leaveEvent(QEvent *event)
{
    QDockWidget::leaveEvent(event);
    m_colorPatchPopup->hide();
}

void EXColorMixerDock::onColorSpaceSelected(const KoColorSpace *colorSpace)
{
    auto &settings = EXSettingsState::instance()->globalSettings;
    if (!settings.useLayerColorSpace) {
        EXColorMixState::instance()->setColorSpace(colorSpace);
        settings.customColorSpace = colorSpace;
        settings.writeAll();
    }
}

void EXColorMixerDock::updateSliders()
{
    EXColorMixState* clrMixState = EXColorMixState::instance();
    EXSettingsState* settingsState = EXSettingsState::instance();
    EXPerColorModelSettings &settings = settingsState->settings[clrMixState->colorModel()->id()];
    if (settings.slidersEnabled) {
        auto sliders = QVector(settings.extraSliders);
        sliders.prepend(clrMixState->colorModel()->id());
        m_sliders->resetColorModels(sliders);
    } else {
        m_sliders->resetColorModels(settings.extraSliders);
    }

    for (auto sliders : m_sliders->sliders()) {
        for (auto slider : sliders->sliders()) {
            clrMixState->connectChannelSlider(slider);
            settingsState->connectChannelSlider(slider);
            m_colorPatchPopup->connectToWidget(slider->bar());
        }
    }
}



