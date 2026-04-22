


#include <string>
#include <charconv>
#include <qbitarray.h>
#include <qvector.h>
#include <qvector3d.h>
#include <QSettings>

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

static QString groupname() { return  "EXColorPresets" ".activePreset"; }
static QString entryname1(int k) { return  "EXColorPresets" ".Preset" + QString::number(k) + ".colorModel"; }
static QString entryname2(int k) { return  "EXColorPresets" ".Preset" + QString::number(k) + ".mixFromGradients"; }
static QString entryname3(int k) { return  "EXColorPresets" ".Preset" + QString::number(k) + ".ingredientMixColors"; }


EXColorPresetStore::EXColorPresetStore()
    : m_configGroup(KSharedConfig::openConfig()->group(EXSettingsGroupName))
    , m_activePreset(0)
    , m_colorMixPresets{}
    , saveSettingsDeferrer(new QTimer(this))
{
    QSettings settings("KritaExtension", "MidiGuiListener");
    m_activePreset = settings.value(groupname()).toInt();

    // A sensible solution with overloading QtDataStream would be too much boilerplate and probably still brittle
    std::array<EXColorPreset, 8> colorMixPresets = {};
    for (size_t k1=0; k1<8; ++k1) {

        QString colorModelName = settings.value(entryname1(k1)).toString();

        ColorModelId cmId;
        if (!EXColorModel::modelIdFromName(colorModelName, cmId)) {
            cmId = ColorModelId::Lab;
        }
        colorMixPresets[k1].m_colorModel = ColorModelFactory::fromId(cmId);

        colorMixPresets[k1].m_mixFromGradients = settings.value(entryname2(k1)).toBool();

        size_t len = settings.beginReadArray(entryname3(k1));
        for (size_t k2=0; k2<len; ++k2)
        {
            settings.setArrayIndex(k2);
            float a = settings.value("value1").toFloat();
            float b = settings.value("value2").toFloat();
            float c = settings.value("value3").toFloat();

            QVector3D mixClr = QVector3D(a,b,c);
            colorMixPresets[k1].m_ingredientMixColors[k2] = mixClr;

            qDebug() << "Reading Preset" << k1 << "Color" << k2 <<
                        QString(": (%1,%2,%3)").arg(mixClr[0]).arg(mixClr[1]).arg(mixClr[2]);
        }
        settings.endArray();

    }
    m_colorMixPresets = colorMixPresets;

    connect(saveSettingsDeferrer, &QTimer::timeout, this, [this]() {
        if (this->presetsChanged) { writeSettings(); presetsChanged = false; }
    });
    saveSettingsDeferrer->start(500);
}

EXColorPresetStore::~EXColorPresetStore() {}

const EXColorPreset& EXColorPresetStore::activePreset() {
    return this->m_colorMixPresets[this->m_activePreset];
}

int EXColorPresetStore::activePresetIndex() {
    return this->m_activePreset;
}

void EXColorPresetStore::writeSettings()
{
    qDebug() << "Saving Preset Settings...";

    QSettings settings("KritaExtension", "MidiGuiListener");
    settings.setValue(groupname(), m_activePreset);

    size_t k1max = m_colorMixPresets.size();
    for (size_t k1=0; k1<k1max; ++k1)
    {
        settings.setValue(entryname1(k1), m_colorMixPresets[k1].m_colorModel->displayName());

        settings.setValue(entryname2(k1), m_colorMixPresets[k1].m_mixFromGradients);

        settings.beginWriteArray(entryname3(k1));
        size_t k2max = m_colorMixPresets[k1].m_ingredientMixColors.size();
        for (size_t k2=0; k2<k2max; ++k2)
        {
            QVector3D val = m_colorMixPresets[k1].m_ingredientMixColors[k2];
            QList<float> valList{val[0], val[1], val[2]};

            qDebug() << "Saving Preset" << k1 << "Color" << k2 <<
                        QString(": (%1,%2,%3)").arg(val[0]).arg(val[1]).arg(val[2]);

            settings.setArrayIndex(k2);
            settings.setValue("value1", val[0]);
            settings.setValue("value2", val[1]);
            settings.setValue("value3", val[2]);
        }
        settings.endArray();
    }
}




void EXColorPresetStore::onPresetSelected(int newPreset)
{
    m_activePreset = newPreset;
    presetsChanged = true;
}

void EXColorPresetStore::onColorSpaceSelected(ColorModelId newClrModel)
{
    m_colorMixPresets[m_activePreset].m_colorModel = ColorModelFactory::fromId(newClrModel);
    presetsChanged = true;
}

void EXColorPresetStore::onGradientModeSelected(bool mixFromGradients)
{
    m_colorMixPresets[m_activePreset].m_mixFromGradients = mixFromGradients;
    presetsChanged = true;
}

void EXColorPresetStore::onMixColorChanged(int clrChannelIndex, QVector3D newClr)
{
    m_colorMixPresets[m_activePreset].m_ingredientMixColors[clrChannelIndex] = newClr;
    presetsChanged = true;
}

