


#include <array>
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

        size_t k2max = colorMixPresets[k1].m_ingredientMixColors.size();
        settings.beginReadArray(entryname3(k1));
        for (size_t k2=0; k2<k2max; ++k2)
        {
            settings.setArrayIndex(k2);
            float a = settings.value("value1").toFloat();
            float b = settings.value("value2").toFloat();
            float c = settings.value("value3").toFloat();

            QVector3D mixClr = QVector3D(a,b,c);
            colorMixPresets[k1].m_ingredientMixColors[k2] = mixClr;

            qDebug() << "Reading Preset" << k1 << "Color" << k2 <<
                        QString(": (%1,%2,%3)").arg(mixClr[0]).arg(mixClr[1]).arg(mixClr[2]);

            colorMixPresets[k1].m_useGradients[k2] = settings.value("useGradient", false).toBool();

            size_t k3max = settings.beginReadArray("Gradients");
            for (size_t k3=0; k3<k3max; ++k3) {
                settings.setArrayIndex(k3);
                float key  = settings.value("Position", -1.0).toFloat();
                if (key >= 0 && key <= 1) {
                    float val1 = settings.value("value1", 0.0).toFloat();
                    float val2 = settings.value("value2", 0.0).toFloat();
                    float val3 = settings.value("value3", 0.0).toFloat();
                    QVector3D val = QVector3D(val1, val2, val3);
                    EXGradientColor clr = EXGradientColor(val, key);
                    colorMixPresets[k1].m_mixGradients[k2].insert(clr);
                }
            }
            settings.endArray();
        }
        settings.endArray();

    }
    m_colorMixPresets = colorMixPresets;

    connect(saveSettingsDeferrer, &QTimer::timeout, this, [this]() {
        if (this->presetsChanged) { writeSettings(); presetsChanged = false; }
    });
    saveSettingsDeferrer->start(500);
    qDebug() << "Reading Settings completed!";
}

EXColorPresetStore::~EXColorPresetStore() {}

const EXColorPreset& EXColorPresetStore::activePreset() const {
    return this->m_colorMixPresets[this->m_activePreset];
}

int EXColorPresetStore::activePresetIndex() const {
    return this->m_activePreset;
}

bool EXColorPresetStore::activePresetUsesGradient(int channelIndex) const {
    return this->m_colorMixPresets[m_activePreset].m_useGradients[channelIndex];
}


void EXColorPresetStore::moveGradientPoint(int channelIndex, int gradientpointIndex, float newPosition) {
    if (inbounds(m_colorMixPresets, m_activePreset)) {
        if (inbounds(m_colorMixPresets[m_activePreset].m_mixGradients, channelIndex)) {
            EXKBSpline& colorSpline = m_colorMixPresets[m_activePreset].m_mixGradients[channelIndex].m_colorSpline;
            if (inbounds(colorSpline.points(), gradientpointIndex)) {
                colorSpline.moveGradientPoint(gradientpointIndex, newPosition);
            }
        }
    }
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

            settings.setValue("useGradient", m_colorMixPresets[k1].m_useGradients[k2]);

            const auto interpPoints = m_colorMixPresets[k1].m_mixGradients[k2].m_colorSpline.points();
            size_t k3max = interpPoints.size();
            qDebug() << QString("Number of gradient colors: %1").arg(k3max);
            settings.beginWriteArray("Gradients", k3max);
            for (size_t k3=0; k3<k3max; ++k3) {
                float key = interpPoints[k3].m_positionOnGradient;
                QVector3D val = interpPoints[k3].m_color;
                settings.setArrayIndex(k3);
                settings.setValue("Position", key);
                settings.setValue("value1", val[0]);
                settings.setValue("value2", val[1]);
                settings.setValue("value3", val[2]);
            }
            settings.endArray();

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

void EXColorPresetStore::onGradientModeSelected(int channel, bool mixFromGradients)
{
    qDebug() << QString("In EXColorPresetStore, onGradientModeSelected: %1 %2").arg(channel).arg(mixFromGradients);
    if ((channel < 0) || (channel >= (int)m_colorMixPresets[m_activePreset].m_useGradients.size())){
        qDebug() << QString("Selected channel %1 out of bounds").arg(channel);
        return;
    } else {
        QString arg; if (mixFromGradients) { arg = "ON"; } else { arg = "OFF"; }
        qDebug() << QString("In Preset %1, Selected channel %2 Gradient mode: %3").arg(m_activePreset).arg(channel).arg(arg);
    };
    m_colorMixPresets[m_activePreset].m_useGradients[channel] = mixFromGradients;
    presetsChanged = true;
}

void EXColorPresetStore::onMixColorChanged(int clrChannelIndex, QVector3D newClr)
{
    m_colorMixPresets[m_activePreset].m_ingredientMixColors[clrChannelIndex] = newClr;
    presetsChanged = true;
}

void EXColorPresetStore::onGradientColorChanged(int clrChannelIndex, int gradientPointIndex, const QVector3D& newlyPickedColor)
{
    EXKBSpline colorSpline = m_colorMixPresets[m_activePreset].m_mixGradients[clrChannelIndex].m_colorSpline;
    colorSpline.replaceColor(gradientPointIndex, newlyPickedColor);
    presetsChanged = true;
}






