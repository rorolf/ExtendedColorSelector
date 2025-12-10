


#ifndef COLORMIXSTATE_H
#define COLORMIXSTATE_H

#include <QObject>
#include <QVector2D>
#include <QVector3D>


class EXColorMixState : public QObject, public KisShared
{
    Q_OBJECT

public:
    EXColorMixState();
    ~EXColorMixState() override = default;

    void setPrimaryChannelValue(float value);
    void setSecondaryChannelValues(const QVector2D &values);
    quint32 primaryChannelIndex() const;
    void setPrimaryChannelIndex(quint32 index);

    QVector3D color() const;
    QColor qColor() const;
    KoColor koColor() const;
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

    static EXColorState *instance();

Q_SIGNALS:
    void sigColorChanged(const QVector3D &color);
    void sigPrimaryChannelIndexChanged(quint32 index);
    void sigColorModelChanged(ColorModelId id);
    void sigColorSpaceChanged(const KoColorSpace *colorSpace);

public Q_SLOTS:
    void onDisplayConfigChanged();

private:
    QVector3D m_kritaBaseColor;
    QVector3D m_mixResultColor;
    QVector3D* m_mixIngredientColors; // always 8
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

typedef KisSharedPtr<EXColorState> EXColorStateSP;




#endif // COLORMIXSTATE_H


