

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

#include "EXColorMixState.h"
#include "EXColorPresetStore.h"
#include "EXColorModel.h"
#include "EXColorPatchWidget.h"
#include "kis_shared_ptr.h"
#include "EXColorMixerDock.h"

EXColorMixerDock::EXColorMixerDock()
    : QDockWidget("Extended Color Selector")
    , m_canvas(nullptr)
    , m_selectedColorPatchWidget(-1)
    , m_colorState(EXColorState::instance())
    , m_colorPresets(EXColorPresetStore::instance())
    , m_settingsState(EXSettingsState::instance())
{
    m_canvas = nullptr;
    auto mainLayout = new QVBoxLayout();

    m_colorPatchPopup = new EXColorPatchPopup(this);
    connect(m_colorState.data(), &EXColorState::sigColorChanged, this, [this]() {
        m_colorPatchPopup->updateColor(m_colorState->qColor());
    });

    auto presetSpaceLayout = new QHBoxLayout(this);
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
        m_colorSpaceSelector->addItem(modelName, QVariant(static_cast<int>(clrid)));
    }
    connect(
        m_colorSpaceSelector, QOverload<int>::of(&QComboBox::currentIndexChanged),
        this, [this](int newIndex) {
            auto data = m_colorSpaceSelector->itemData(newIndex);
            if (data.isValid())
            {
                ColorModelId newClrId = static_cast<ColorModelId>(data.value<int>());
                m_colorMixState->setColorModel(newClrId);
            }
        }
    );

    connect(m_colorState.data(), &EXColorState::sigColorSpaceChanged, this, [this](const KoColorSpace *colorSpace) {

        auto newColorModel = ColorModelFactory::fromKoColorSpace(colorSpace);
        ColorModelId newClrId = newColorModel->id();
        delete newColorModel;
        ColorModelId oldClrId = static_cast<ColorModelId>(m_colorSpaceSelector->currentData().value<int>());
        if (newClrId != oldClrId) {
            int newIndex = m_colorSpaceSelector->findData(newClrId);
            if (newIndex >= 0)
            {
                m_colorSpaceSelector->blockSignals(true);
                m_colorSpaceSelector->setCurrentIndex(newIndex);
                m_colorSpaceSelector->blockSignals(false);
            }
        }
        m_colorSpaceSelectorButton->setText(colorSpace->name());
    });

    //m_colorSpaceSelectorButton->setPopupWidget(m_colorSpaceSelector);
    //m_colorSpaceSelectorButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    presetSpaceLayout->addWidget(m_presetSelector);
    presetSpaceLayout->addWidget(colorSpaceSelectorLabel);
    presetSpaceLayout->addWidget(m_colorSpaceSelector);
    mainLayout->addLayout(presetSpaceLayout);

    auto mixPresetLayout = new QHBoxLayout(this);
    auto mixChannelLayout = new QGridLayout(this);
    auto mixSideLayout = new QVBoxLayout(this);

    for (int k = 0; k<9; ++k)
    {
        auto newChannelWidget = new EXColorPatchWidget();
        m_colorPatchWidgets[k] = newChannelWidget;

        if (k!=4)
        {
            connect(newChannelWidget, &EXColorPatchWidget::sigClicked,
                this, [this, k]() {
                    if (this->m_selectedColorPatchWidget != k)
                    {
                        this->m_selectedColorPatchWidget = k;
                        this->m_colorPatchWidgets[k]->m_selected = true;
                    }
                    else
                    {
                        this->m_selectedColorPatchWidget = -1;
                        this->m_colorPatchWidgets[k]->m_selected = false;
                    }
                }
            );
        }
        else
        {
            connect(m_colorMixState.data(), &EXColorMixState::sigKritaBaseColorChanged,
                this, [this, newChannelWidget](QVector3D newClr) {
                    newChannelWidget->m_color = m_colorMixState->toQColor(newClr);
                }
            );
        }

        int x = k%3;
        int y = k/3;
        mixChannelLayout->addWidget(newChannelWidget, x, y);
    }

    QButtonGroup *mixerModeButtonGroup = new QButtonGroup(this);

    QRadioButton *mixFromColorsButton = new QRadioButton(this);
    mixFromColorsButton->setWhatsThis("MixColor");
    mixFromColorsButton->setChecked(true);

    connect(mixFromColorsButton, &QRadioButton::clicked,
        this, [this]() {
            size_t activePresetN = m_colorPresets->m_activePreset;
            auto activePreset = &m_colorPresets->m_colorMixPresets[activePresetN];
            activePreset->m_mixFromGradients = false;
        }
    );

    QRadioButton *mixFromGradientButton = new QRadioButton(this);
    mixFromGradientButton->setWhatsThis("Gradient");
    connect(mixFromGradientButton, &QRadioButton::clicked,
        this, [this]() {
            size_t activePresetN = m_colorPresets->m_activePreset;
            auto activePreset = &m_colorPresets->m_colorMixPresets[activePresetN];
            activePreset->m_mixFromGradients = true;
        }
    );

    mixerModeButtonGroup->addButton(mixFromColorsButton, 0);
    mixerModeButtonGroup->addButton(mixFromGradientButton, 1);

    auto mixResultColorPatch = new EXColorPatchWidget();

    connect(m_colorMixState, &EXColorMixState::sigColorChanged,
        this, [this, &mixResultColorPatch](QVector3D newClrV) {
            auto newClr = m_colorMixState->toQColor(newClrV);
            mixResultColorPatch->onColorSelected(newClr);
        }
    );

    mixSideLayout->addWidget(mixFromColorsButton);
    mixSideLayout->addWidget(mixFromGradientButton);
    mixSideLayout->addWidget(mixResultColorPatch);

    mixPresetLayout->addLayout(mixChannelLayout);
    mixPresetLayout->addLayout(mixSideLayout);

    mainLayout->addLayout(mixChannelLayout);


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

    connect(m_colorState.data(), &EXColorState::sigColorSpaceChanged, this, [this](const KoColorSpace *colorSpace) {
        if (colorSpace != m_colorSpaceSelector->currentColorSpace()) {
            m_colorSpaceSelector->setCurrentColorSpace(colorSpace);
        }
        m_colorSpaceSelectorButton->setText(colorSpace->name());
    });
    connect(m_useLayerColorSpaceButton, &QPushButton::toggled, this, [this](bool checked) {
        auto &settings = m_settingsState->globalSettings;
        settings.useLayerColorSpace = checked;
        m_colorState->setUseLayerColorSpace(checked);
        m_colorSpaceSelectorButton->setEnabled(!checked);
        settings.customColorSpace = m_colorSpaceSelector->currentColorSpace();
        m_useLayerColorSpaceButton->setIcon(checked ? KisIconUtils::loadIcon("chain-icon")
                                                    : KisIconUtils::loadIcon("chain-broken-icon"));
        settings.writeAll();
    });
    connect(m_colorSpaceSelector,
            SIGNAL(colorSpaceChanged(const KoColorSpace *)),
            this,
            SLOT(onColorSpaceSelected(const KoColorSpace *))
    );

    m_plane = new EXChannelPlane(this);
    m_plane->setColorModel(ColorModelFactory::fromId((ColorModelId)m_settingsState->globalSettings.currentColorModel));
    m_colorState->connectChannelPlane(m_plane);
    m_settingsState->connectChannelPlane(m_plane);
    m_colorPatchPopup->connectToWidget(m_plane);

    m_sliders = new EXChannelSlidersGroup(QVector<ColorModelId>(), this);
    m_colorModelSwitchers = new EXColorModelSwitchers(m_colorState, m_settingsState, this);

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
    m_colorState->setUseLayerColorSpace(m_settingsState->globalSettings.useLayerColorSpace);
    if (m_settingsState->globalSettings.useLayerColorSpace) {
        m_useLayerColorSpaceButton->setIcon(KisIconUtils::loadIcon("chain-icon"));
    } else {
        m_useLayerColorSpaceButton->setIcon(KisIconUtils::loadIcon("chain-broken-icon"));
        auto customColorSpace = m_settingsState->globalSettings.customColorSpace;
        m_colorSpaceSelector->setCurrentColorSpace(customColorSpace);
        m_colorState->setColorSpace(customColorSpace);
    }

    connect(m_colorState.data(), &EXColorState::sigColorModelChanged, m_plane, [this]() {
        m_settingsState->applySettingsToPlane(m_plane);
    });

    updateSliders();
    connect(m_colorState.data(), &EXColorState::sigColorModelChanged, this, &EXColorSelectorDock::updateSliders);
    connect(m_settingsState.data(), &EXSettingsState::sigSettingsChanged, this, &EXColorSelectorDock::updateSliders);
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
        m_colorState->setCanvas(m_canvas);
        Q_EMIT m_settingsState->sigSettingsChanged();
    }
}

void EXColorSelectorDock::unsetCanvas()
{
    m_canvas = nullptr;
    m_plane->setCanvas(nullptr);
    m_sliders->setCanvas(nullptr);
    m_colorState->setCanvas(nullptr);
    m_portableSelector->setCanvas(nullptr);
}

void EXColorSelectorDock::enterEvent(QEvent *event)
{
    QDockWidget::enterEvent(event);
    m_colorPatchPopup->recordColor(m_colorState->qColor());
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
        m_colorState->setColorSpace(colorSpace);
        settings.customColorSpace = colorSpace;
        settings.writeAll();
    }
}

void EXColorSelectorDock::updateSliders()
{
    auto &settings = m_settingsState->settings[m_colorState->colorModel()->id()];
    if (settings.slidersEnabled) {
        auto sliders = QVector(settings.extraSliders);
        sliders.prepend(m_colorState->colorModel()->id());
        m_sliders->resetColorModels(sliders);
    } else {
        m_sliders->resetColorModels(settings.extraSliders);
    }

    for (auto sliders : m_sliders->sliders()) {
        for (auto slider : sliders->sliders()) {
            m_colorState->connectChannelSlider(slider);
            m_settingsState->connectChannelSlider(slider);
            m_colorPatchPopup->connectToWidget(slider->bar());
        }
    }
}



