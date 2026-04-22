


#ifndef COLOR_PRESET_STORE_H
#define COLOR_PRESET_STORE_H

#include <QObject>
#include <QVector2D>
#include <QVector3D>
#include <QBitArray>
#include <array>
#include <QTimer>

#include <kconfiggroup.h>

#include <KoColor.h>
#include <KoColorSpace.h>
#include <kis_shared.h>
#include <kis_shared_ptr.h>
#include <qvector3d.h>

#include "EXColorModel.h"


class EXColorPreset
{
    public:
        EXColorPreset() {};
        //~EXColorPreset() override = default;

        ColorModelSP m_colorModel;
        // TODO: How to replace the colorspace pointer?
        //const KoColorSpace *m_currentColorSpace;

        bool m_mixFromGradients; // length 8
        std::array<QVector3D, 8> m_ingredientMixColors;
        //std::array<std::vector<QVector3D>, 7> m_ingredientMixGradients;
        //std::array<std::vector<float>, 7> m_ingredientGradientStopPositions;
        //float m_mixFromGradientStrength;

    private:
};


class EXColorPresetStore : public QObject, public KisShared
{
    Q_OBJECT

public:
    EXColorPresetStore();
    ~EXColorPresetStore();

    KConfigGroup m_configGroup;

    const EXColorPreset& activePreset();
    int activePresetIndex();
    void writeSettings();

    static EXColorPresetStore *instance();

Q_SIGNALS:
    void sigColorPresetChanged();        // overwrites EXColorState's active preset

public Q_SLOTS:
    void onPresetSelected(int newPreset);
    void onColorSpaceSelected(ColorModelId newClrModel);
    void onGradientModeSelected(bool mixFromGradients);
    void onMixColorChanged(int clrChannelIndex, QVector3D newClr);

private:
    int m_activePreset;
    std::array<EXColorPreset, 8> m_colorMixPresets;
    QTimer* saveSettingsDeferrer;
    bool presetsChanged = false;
};

typedef KisSharedPtr<EXColorPresetStore> EXColorPresetStoreSP;


#endif // ColorPresetStore
