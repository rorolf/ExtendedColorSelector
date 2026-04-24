#include <algorithm>
#include <kis_canvas2.h>
#include <kis_display_color_converter.h>
#include <qvector.h>
#include <qvector3d.h>


#include "EXColorMixState.h"
#include "EXColorModel.h"
#include "EXColorPresetStore.h"
#include "EXSettingsState.h"
#include "EXUtils.h"
#include "KoColor.h"

#include <QtGlobal>
#include <cmath>

static EXColorMixState *s_instance = nullptr;

EXColorMixState *EXColorMixState::instance()
{
    if (!s_instance) {
        s_instance = new EXColorMixState();
    }
    return s_instance;
}

EXColorMixState::EXColorMixState()
    : m_color(1, 1, 1)
    , m_kritaBaseColor(1,1,1) // elements are probably float since e.g. x() component is float
    , m_mixResultColor(1,1,1)
    , m_mixIngredientColors{}
    //float m_kritaBaseColorWeight; // implicitly set to a fixed value
    , m_mixIngredientColorWeights{}

    , m_primaryChannelIndex(0)
    , m_colorModel(
          ColorModelFactory::fromId((ColorModelId)EXSettingsState::instance()->globalSettings.currentColorModel))
    , m_currentColorSpace(nullptr)
    , m_resourceProvider(nullptr)
    , m_dri(nullptr)
    , m_dcc(nullptr)
    , m_koColorConverter(nullptr)
    , m_blockColorSync(false)
    , m_useLayerColorSpace(false)

{
}

void EXColorMixState::setColorModel(ColorModelId model)
{
    if (m_colorModel->id() == model) {
        return;
    }

    auto &settings = EXSettingsState::instance()->globalSettings;
    settings.currentColorModel = model;
    settings.writeAll();

    m_primaryChannelIndex = EXSettingsState::instance()->settings[model].primaryIndex;
    Q_EMIT sigPrimaryChannelIndexChanged(m_primaryChannelIndex);

    auto newModel = ColorModelFactory::fromId(model);

    //m_color = m_colorModel->transferTo(newModel, m_color, m_color);
    //ExtendedUtils::saturateColor(m_color);

    m_kritaBaseColor = m_colorModel->transferTo(newModel, m_kritaBaseColor, m_kritaBaseColor);
    ExtendedUtils::saturateColor(m_kritaBaseColor);
    for (int k = 0; k<8; ++k) {
        m_mixIngredientColors[k] = m_colorModel->transferTo(newModel, m_mixIngredientColors[k], m_mixIngredientColors[k]);
        ExtendedUtils::saturateColor(m_mixIngredientColors[k]);
    }

    m_colorModel = newModel;
    m_koColorConverter = new EXKoColorConverter(m_currentColorSpace);

    Q_EMIT sigColorModelChanged(model);
    this->mixColors();
}

void EXColorMixState::blockUpdates(bool blockUpdates) {
    this->m_blockColorSync = blockUpdates;
}

void EXColorMixState::mixColors()
{
    // TODO: allow multiple mixing behaviors
    // TODO: differentiate between mix and gradient presets

    QVector3D out_color = m_kritaBaseColor;

    QVector3D mixColor = QVector3D(0,0,0);
    float totalMixWight = 0.0;
    float finalMixWeight = 0.0;
    for (size_t k=0; k<m_mixIngredientColors.size(); ++k)
    {
        float mixWeight = m_mixIngredientColorWeights[k];
        totalMixWight += mixWeight;
        finalMixWeight = qMax(finalMixWeight, mixWeight);
        mixColor += m_mixIngredientColors[k]*mixWeight;
    }
    if (totalMixWight > 0.005) {
        mixColor /= totalMixWight;
        out_color = out_color  +  (mixColor-out_color) * finalMixWeight;
    }
    ExtendedUtils::saturateColor(out_color);

    qDebug() << "Color changed from" << m_color << "to" << out_color;
    if (m_color != out_color)
    {
        m_color = out_color;
        this->sendToKrita();
        Q_EMIT sigColorChanged(m_color);
    }
}

const ColorModelSP EXColorMixState::colorModel() const
{
    return m_colorModel;
}

void EXColorMixState::sendToKrita()
{
    QVector3D currentColor = m_colorModel->transferTo(kritaColorModel(), m_color);
    ExtendedUtils::saturateColor(currentColor);

    m_blockColorSync = true;
    m_resourceProvider->setFGColor(m_koColorConverter->displayChannelsToKoColor(QVector4D(currentColor, 1.0f)));
    m_blockColorSync = false;
}

void EXColorMixState::syncFromKrita()
{
    if (m_blockColorSync || !m_resourceProvider || !m_currentColorSpace || !m_colorModel) {
        return;
    }

    KoColor koColor = m_resourceProvider->fgColor();
    koColor.convertTo(m_currentColorSpace);
    QVector3D newColor = m_koColorConverter->koColorToDisplayChannels(koColor).toVector3D();
    //m_color = kritaColorModel()->transferTo(m_colorModel, newColor, m_color);
    //setColor(m_color);

    if (newColor != m_kritaBaseColor) {
        newColor = kritaColorModel()->transferTo(m_colorModel, newColor, m_kritaBaseColor);
        setColor(newColor);
    }

}

void EXColorMixState::setCanvas(KisCanvas2 *canvas)
{
    if (canvas) {
        m_resourceProvider = canvas->imageView()->resourceProvider();
        m_dcc = canvas->displayColorConverter();
        m_dri = canvas->displayColorConverter()->displayRendererInterface();

        connect(m_resourceProvider, &KisCanvasResourceProvider::sigFGColorChanged, this, &EXColorMixState::syncFromKrita);

        if (m_useLayerColorSpace) {
            setColorSpace(m_dcc->paintingColorSpace());
            connect(m_dcc,
                    &KisDisplayColorConverter::displayConfigurationChanged,
                    this,
                    &EXColorMixState::onDisplayConfigChanged,
                    Qt::UniqueConnection);
        } else {
            syncFromKrita();
        }
        emit sigCanvasReady();
    }
}

void EXColorMixState::setPrimaryChannelValue(float value)
{
    // m_color[m_primaryChannelIndex] = value;
    QVector3D newClr = m_color;
    newClr[m_primaryChannelIndex] = value;
    setColor(m_color);
}

quint32 EXColorMixState::primaryChannelIndex() const
{
    return m_primaryChannelIndex;
}

void EXColorMixState::setPrimaryChannelIndex(quint32 index)
{
    Q_ASSERT(index < 3);

    auto &settings = EXSettingsState::instance()->settings[m_colorModel->id()];
    settings.primaryIndex = index;
    settings.writeAll();

    m_primaryChannelIndex = index;

    Q_EMIT sigPrimaryChannelIndexChanged(index);
}

void EXColorMixState::setSecondaryChannelValues(const QVector2D &values)
{
    QVector3D newClr = m_color; // creates a copy on write
    switch (m_colorModel->channelCount()) {
    case 2:
        newClr = values.toVector3D();
        break;
    case 3:
        switch (m_primaryChannelIndex) {
        case 0:
            newClr[1] = values.x();
            newClr[2] = values.y();
            break;
        case 1:
            newClr[0] = values.x();
            newClr[2] = values.y();
            break;
        case 2:
            newClr[0] = values.x();
            newClr[1] = values.y();
            break;
        }
        break;
    }

    setColor(newClr);
}

QVector3D EXColorMixState::color() const
{
    return m_color;
}

KoColor EXColorMixState::toKoColor(const QVector3D &color) const
{
    auto kritaColor = m_colorModel->transferTo(kritaColorModel(), color);
    return m_koColorConverter->displayChannelsToKoColor(QVector4D(kritaColor, 1.0f));
}

QColor EXColorMixState::toQColor(const QVector3D &color) const
{
    if (!m_dri) return QColor::fromRgb(0,0,0);
    qDebug() << "Converting to KoColor...";
    auto koClr = this->toKoColor(color);
    qDebug() << "Displayrenderer creates QColor...";
    return m_dri->toQColor(koClr);
}

KoColor EXColorMixState::koColor() const
{
    return this->toKoColor(m_color);
}

QColor EXColorMixState::qColor() const
{
    return this->toQColor(m_color);
}

void EXColorMixState::setColor(const QVector3D &color)
{
    if (m_blockColorSync) { return; }
    m_kritaBaseColor = color;
    Q_EMIT sigKritaBaseColorChanged(m_kritaBaseColor);
    this->mixColors();
}

const KoColorSpace *EXColorMixState::colorSpace() const
{
    return m_currentColorSpace;
}

const ColorModelSP EXColorMixState::kritaColorModel() const
{
    return m_koColorConverter->colorModel();
}

const EXColorConverterSP EXColorMixState::koColorConverter() const
{
    return m_koColorConverter;
}

void EXColorMixState::setUseLayerColorSpace(bool use)
{
    m_useLayerColorSpace = use;
    if (!m_dcc) {
        return;
    }

    if (use) {
        setColorSpace(m_dcc->paintingColorSpace());
        connect(m_dcc,
                &KisDisplayColorConverter::displayConfigurationChanged,
                this,
                &EXColorMixState::onDisplayConfigChanged,
                Qt::UniqueConnection);
    } else {
        disconnect(m_dcc, nullptr, this, nullptr);
    }
}

void EXColorMixState::setColorSpace(const KoColorSpace *colorSpace)
{
    m_currentColorSpace = colorSpace;
    m_koColorConverter = new EXKoColorConverter(colorSpace);

    syncFromKrita();
    Q_EMIT sigColorSpaceChanged(m_currentColorSpace);
}

void EXColorMixState::onDisplayConfigChanged()
{
    if (m_useLayerColorSpace && m_dcc) {
        setColorSpace(m_dcc->paintingColorSpace());
    }
}

void EXColorMixState::onColorPresetChanged()
{
    qDebug() << "EXColorMixState fired onColorPresetChanged";
    EXColorPreset newPreset = EXColorPresetStore::instance()->activePreset();
    ColorModelId newColorModel = newPreset.m_colorModel->id();
    this->setColorModel(newColorModel);
    // TODO: Find out which KoColorSpace to use
    //this->setColorSpace()
    m_mixIngredientColors = newPreset.m_ingredientMixColors;
    // m_mixFromGradients = nextPreset.m_mixFromGradients;
    // TODO: copy Gradients over


    mixColors();
}
void EXColorMixState::onIngredientColorWeightChanged(int weightIndex, float value)
{
    qDebug() << "Changing mixIngredientColorWeight" << weightIndex << "to" << value;
    m_mixIngredientColorWeights[weightIndex] = value;
    mixColors();
}

//TODO: change UI, and change these methods
//TODO: move logic into UI

void EXColorMixState::connectChannelPlane(EXChannelPlane *plane)
{
    // Assume the color model inside the plane is always the same as we use here.
    plane->setColorModel(m_colorModel);
    plane->setColor(m_color, m_colorModel);
    plane->setColorConverter(m_koColorConverter);
    connect(plane, &EXChannelPlane::sigPrimaryChannelValueSelected, this, &EXColorMixState::setPrimaryChannelValue);
    connect(plane, &EXChannelPlane::sigSecondaryChannelsValueSelected, this, &EXColorMixState::setSecondaryChannelValues);
    //TODO: logic error, needs to send to EXMixColorState, and then mixColors sends to Krita
    connect(plane, &EXChannelPlane::sigValueFinalized, this, &EXColorMixState::sendToKrita);
    connect(this, &EXColorMixState::sigColorChanged, plane, [this, plane](QVector3D color) {
        plane->setColor(color, m_colorModel);
    });
    connect(this, &EXColorMixState::sigColorModelChanged, plane, [this, plane]() {
        plane->setColorModel(m_colorModel);
        plane->setColor(m_color, m_colorModel);
    });
    connect(this, &EXColorMixState::sigColorSpaceChanged, plane, [this, plane](const KoColorSpace *) {
        plane->setColorConverter(m_koColorConverter);
    });
    connect(this, &EXColorMixState::sigPrimaryChannelIndexChanged, plane, [plane](quint32 index) {
        plane->setPrimaryChannelIndex(index);
    });
}

void EXColorMixState::connectChannelSlider(EXChannelSlider *slider)
{
    auto result = slider->colorModelAndChannelIndex();
    auto colorModel = result.first;
    auto channelIndex = result.second;
    slider->setColorConverter(m_koColorConverter);
    slider->setColor(m_color, m_colorModel);
    slider->setActive(colorModel->id() == m_colorModel->id());
    slider->setSelected(channelIndex == m_primaryChannelIndex);

    connect(slider->bar(), &EXChannelSliderBar::sigValueChanging, this, [this, colorModel, slider]() {
        setColor(colorModel->transferTo(m_colorModel.data(), slider->colorAtCurrentModel(), m_color));
    });
    //TODO: logic error
    connect(slider->bar(), &EXChannelSliderBar::sigValueFinalized, this, &EXColorMixState::sendToKrita);
    connect(this, &EXColorMixState::sigColorChanged, slider, [this, slider](QVector3D color) {
        slider->setColor(color, m_colorModel);
    });
    connect(this, &EXColorMixState::sigColorModelChanged, slider, [this, colorModel, slider](ColorModelId modelId) {
        slider->setColor(m_color, m_colorModel);
        slider->setActive(colorModel->id() == modelId);
    });
    connect(this, &EXColorMixState::sigColorSpaceChanged, slider, [this, slider](const KoColorSpace *) {
        slider->setColorConverter(m_koColorConverter);
    });
    connect(this, &EXColorMixState::sigPrimaryChannelIndexChanged, slider, [channelIndex, slider](quint32 index) {
        slider->setSelected(channelIndex == index);
    });
    connect(slider, &EXChannelSlider::sigSelected, this, [this, channelIndex]() {
        setPrimaryChannelIndex(channelIndex);
    });
}


