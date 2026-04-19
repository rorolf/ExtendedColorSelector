


#include <string>
#include <charconv>
#include <qbitarray.h>
#include <qvector.h>
#include <qvector3d.h>


#include "EXColorPresetStore.h"
#include "EXColorModel.h"
#include "EXSettingsState.h"
#include "EXUtils.h"


static EXColorPresetStore *s_instance = nullptr;

EXColorPresetStore *EXColorPresetStore::instance()
{
    if (!s_instance)
    {
        s_instance = new EXColorPresetStore();
    }
    return s_instance;
}


EXColorPresetStore::EXColorPresetStore()
    : m_configGroup(KSharedConfig::openConfig()->group(EXSettingsGroupName))
    , m_activePreset(0)
    , m_colorMixPresets{}
    , m_selectedColorMixChannel(-1)
{
    m_activePreset = m_configGroup.readEntry("EXColorPresets" ".activePreset", 0);

    //m_colorMixPresets = m_configGroup.readEntry("EXColorPreset" ".colorMixPresets",{});
    // A sensible solution with overloading QtDataStream would be too much boilerplate and probably still brittle
    std::array<EXColorPreset, 8> colorMixPresets = {};
    for (size_t k1=0; k1<8; ++k1)
    {
        QString colorModelName = m_configGroup.readEntry(
            "EXColorPresets"
            ".Preset" + QString::number(k1) +
            ".colorModel"
            , ""
        );

        ColorModelId cmId;
        if (!EXColorModel::modelIdFromName(colorModelName, cmId))
        {
            cmId = ColorModelId::Lab;
        }
        colorMixPresets[k1].m_colorModel = ColorModelFactory::fromId(cmId);

        colorMixPresets[k1].m_mixFromGradients = m_configGroup.readEntry(
            "EXColorPresets"
            ".Preset" + QString::number(k1) +
            ".mixFromGradients"
            , false
        );

        std::array<QVector3D, 8> ingredientMixColors = colorMixPresets[k1].m_ingredientMixColors;
        for (size_t k2=0; k2<8; ++k2)
        {
            QList<float> deflt{0.0f, 0.0f, 0.0f};
            QList<float> mixClr = m_configGroup.readEntry(
                "EXColorPresets"
                ".Preset" + QString::number(k1) +
                ".ingredientMixColor" + QString::number(k2)
                , deflt
            );
            ingredientMixColors[k2] = QVector3D(mixClr[0],mixClr[1],mixClr[2]);
        }
    }
    m_colorMixPresets = colorMixPresets;
    m_selectedColorMixChannel = m_configGroup.readEntry("EXColorPresets" ".selectedColorMixChannel",-1);
}


void EXColorPresetStore::writeSettings()
{
    qDebug() << "Saving Preset Settings...";

    m_configGroup.writeEntry("EXColorPresets" ".activePreset", m_activePreset);

    for (size_t k1=0; k1<8; ++k1)
    {
        m_configGroup.writeEntry(
            "EXColorPresets"
            ".Preset" + QString::number(k1) +
            ".colorModel"
            , m_colorMixPresets[k1].m_colorModel->displayName()
        );

        m_configGroup.writeEntry(
            "EXColorPresets"
            ".Preset" + QString::number(k1) +
            ".mixFromGradients"
            , m_colorMixPresets[k1].m_mixFromGradients
        );

        for (size_t k2=0; k2<8; ++k2)
        {
            QVector3D val = m_colorMixPresets[k1].m_ingredientMixColors[k2];
            QList<float> valList{val[0], val[1], val[2]};
            m_configGroup.writeEntry(
                "EXColorPresets"
                ".Preset" + QString::number(k1) +
                ".ingredientMixColor" + QString::number(k2)
                , valList
            );
        }
    }
    m_configGroup.writeEntry("EXColorPresets" ".selectedColorMixChannel", m_selectedColorMixChannel);

    m_configGroup.sync();
}




void EXColorPresetStore::onPresetSelected(int newPreset)
{
    m_activePreset = newPreset;
    writeSettings();
}

void EXColorPresetStore::onMixColorChannelSelected(int newChannel)
{
    m_selectedColorMixChannel = newChannel;
    writeSettings();
}

void EXColorPresetStore::onColorSpaceSelected(ColorModelId newClrModel)
{
    m_colorMixPresets[m_activePreset].m_colorModel = ColorModelFactory::fromId(newClrModel);
    writeSettings();
}

void EXColorPresetStore::onGradientModeSelected(bool mixFromGradients)
{
    m_colorMixPresets[m_activePreset].m_mixFromGradients = mixFromGradients;
    writeSettings();
}

void EXColorPresetStore::onMixColorSelected(int clrChannelIndex, QVector3D newClr)
{
    m_colorMixPresets[m_activePreset].m_ingredientMixColors[clrChannelIndex] = newClr;
    writeSettings();
}

