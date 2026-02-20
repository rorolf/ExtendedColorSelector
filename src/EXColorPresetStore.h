


#ifndef COLOR_PRESET_STORE_H
#define COLOR_PRESET_STORE_H

#include <QObject>
#include <QVector2D>
#include <QVector3D>
#include <QBitArray>
#include <array>

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
    ~EXColorPresetStore() override = default;

    KConfigGroup m_configGroup;

    int m_activePreset;
    std::array<EXColorPreset, 8> m_colorMixPresets;

    int m_selectedColorMixChannel;

    void writeSettings();

    static EXColorPresetStore *instance();

Q_SIGNALS:
    void sigColorPresetChanged();        // overwrites EXColorState's active preset
    // system looks wonky
    void sigMixColorChannelInFocus();    // prevents EXColorState from updating
    void sigMixColorChannelOutOfFocus(); // reenables EXColorState updates

public Q_SLOTS:
    void onPresetSelected(int newPreset);
    void onMixColorChannelSelected(int newChannel);
    void onColorSpaceSelected(ColorModelId newClrModel);
    void onGradientModeSelected(bool mixFromGradients);
    void onMixColorSelected(int clrChannelIndex, QVector3D newClr);

private:

};

typedef KisSharedPtr<EXColorPresetStore> EXColorPresetStoreSP;


#endif // ColorPresetStore
