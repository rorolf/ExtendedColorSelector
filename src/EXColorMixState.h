


#ifndef COLORMIXSTATE_H
#define COLORMIXSTATE_H

#include <QObject>
#include <QVector2D>
#include <QVector3D>
#include <array>

#include <KoColor.h>
#include <KoColorDisplayRendererInterface.h>
#include <KoColorSpace.h>
#include <kis_canvas2.h>
#include <kis_canvas_resource_provider.h>
#include <kis_display_color_converter.h>
#include <kis_shared.h>
#include <kis_shared_ptr.h>

#include "EXChannelPlane.h"
#include "EXChannelSlider.h"
#include "EXColorModel.h"
#include "EXKoColorConverter.h"


class EXColorMixState : public QObject, public KisShared
{
    Q_OBJECT

public:
    EXColorMixState();
    static EXColorMixState *instance();
    ~EXColorMixState() override = default;

    void setPrimaryChannelValue(float value);
    void setSecondaryChannelValues(const QVector2D &values);
    quint32 primaryChannelIndex() const;
    void setPrimaryChannelIndex(quint32 index);

    QVector3D color() const;
    QColor qColor() const;
    KoColor koColor() const;
    KoColor toKoColor(const QVector3D &color) const;
    QColor toQColor(const QVector3D &color) const;
    void setColor(const QVector3D &color);
    const KoColorSpace *colorSpace() const;
    const ColorModelSP kritaColorModel() const;
    const EXColorConverterSP koColorConverter() const;
    void setColorSpace(const KoColorSpace *colorSpace);

    void setColorModel(ColorModelId model);
    const ColorModelSP colorModel() const;
    void setUseLayerColorSpace(bool use);

    void sendToKrita();
    void syncFromKrita();
    void setCanvas(KisCanvas2 *canvas);

    void connectChannelPlane(EXChannelPlane *plane);
    void connectChannelSlider(EXChannelSlider *slider);
    void clearConnectedChannelSliders();

    // additional
    void mixColors();

Q_SIGNALS:
    void sigColorChanged(const QVector3D &color);
    void sigPrimaryChannelIndexChanged(quint32 index);
    void sigColorModelChanged(ColorModelId id);
    void sigColorSpaceChanged(const KoColorSpace *colorSpace);
    void sigKritaBaseColorChanged(const QVector3D &color);

public Q_SLOTS:
    void onDisplayConfigChanged();
    void onColorPresetChanged(int newPresetIndex); //when switching between presets and changing presets themselves
    // a specific onIngredientColorChanged does not exist
    void onIngredientColorWeightChanged(int weightIndex, float value);

private:
    QVector3D m_color;
    QVector3D m_kritaBaseColor; // updates check whether this is equal to kritaBaseColor or mixResultColor
    QVector3D m_mixResultColor;
    std::array<QVector3D, 8> m_mixIngredientColors;
    //float m_kritaBaseColorWeight; // implicitly set to a fixed value
    std::array<float, 8> m_mixIngredientColorWeights;
    // bool m_mixFromGradients;
    // std::array<std::vector<QVector3D>,7> mixGradientColors;
    // std::array<std::vector<float>,7> mixGradientStopPositions;
    // std::array<float,7> mixGradientColorPositions;
    // float mixGradientWeight;

    quint32 m_primaryChannelIndex;
    ColorModelSP m_colorModel;
    const KoColorSpace *m_currentColorSpace;
    KisCanvasResourceProvider *m_resourceProvider;
    KoColorDisplayRendererInterface *m_dri;
    KisDisplayColorConverter *m_dcc;
    EXColorConverterSP m_koColorConverter;
    bool m_blockColorSync;
    bool m_useLayerColorSpace;
};

typedef KisSharedPtr<EXColorMixState> EXColorMixStateSP;




#endif // COLORMIXSTATE_H


