


#ifndef COLOR_PRESET_STORE_H
#define COLOR_PRESET_STORE_H

#include <QObject>
#include <QVector2D>
#include <QVector3D>
#include <QBitArray>
#include <QTimer>
#include <QtGlobal>
#include <array>

#include <kconfiggroup.h>

#include <KoColor.h>
#include <KoColorSpace.h>
#include <kis_shared.h>
#include <kis_shared_ptr.h>
#include <qvector3d.h>

#include "EXColorModel.h"
#include "EXGradient.h"


class EXColorPreset
{
    public:
        EXColorPreset() {};
        //~EXColorPreset() override = default;

        ColorModelSP m_colorModel;
        // TODO: How to replace the colorspace pointer?
        //const KoColorSpace *m_currentColorSpace;

        std::array<QVector3D, 8> m_ingredientMixColors;
        std::array<bool, 8> m_useGradients;
        std::array<EXColorGradient, 8> m_mixGradients;

    private:
};


class EXColorPresetStore : public QObject, public KisShared
{
    Q_OBJECT

public:
    EXColorPresetStore();
    ~EXColorPresetStore();
    static EXColorPresetStore *instance();

    KConfigGroup m_configGroup;

    const EXColorPreset& activePreset() const;
    int activePresetIndex() const;
    bool activePresetUsesGradient(int channel) const;
    void writeSettings();

    void addGradientPoint(int channelIndex, float position);
    void removeGradientPoint(int channelIndex, int gradientpointIndex);
    void moveGradientPoint(int channelIndex, int gradientpointIndex, float newWeight);

Q_SIGNALS:
    void sigColorPresetChanged();        // overwrites EXColorState's active preset

public Q_SLOTS:
    void onPresetSelected(int newPreset);
    void onColorSpaceSelected(ColorModelId newClrModel);
    void onGradientModeSelected(int channel, bool mixFromGradients);
    void onMixColorChanged(int clrChannelIndex, QVector3D newClr);
    void onGradientColorChanged(int clrChannelIndex, int gradientPointIndex, const QVector3D& newlyPickedColor);

private:
    int m_activePreset;
    std::array<EXColorPreset, 8> m_colorMixPresets;
    QTimer* saveSettingsDeferrer;
    bool presetsChanged = false;
};

typedef KisSharedPtr<EXColorPresetStore> EXColorPresetStoreSP;


#endif // ColorPresetStore
