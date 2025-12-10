#########################
## EXKoColorConverter.h
#########################




#ifndef EXKOCOLORCONVERTER_H
#define EXKOCOLORCONVERTER_H

#include <array>

#include <QVector4D>

#include <KoColor.h>
#include <KoColorSpace.h>
#include <kis_shared.h>
#include <kis_shared_ptr.h>

#include "EXColorModel.h"

class EXKoColorConverter : public KisShared
{
public:
    EXKoColorConverter(const KoColorSpace *colorSpace);
    KoColor displayChannelsToKoColor(const QVector4D &channels) const;
    void displayChannelsToKoColor(quint8 *target, const QVector4D &channels, QVector<float> &tempChannelBuffer) const;
    QVector4D koColorToDisplayChannels(const KoColor &color) const;
    QVector4D koColorToDisplayChannels(const KoColor &color, QVector<float> &tempChannelBuffer) const;
    const KoColorSpace *colorSpace() const;
    const ColorModelSP colorModel() const;

private:
    const KoColorSpace *m_colorSpace;
    int m_logicalToMemoryPosition[4];
    bool m_isRGBA;
    bool m_isLinear;
    bool m_applyGamma;
    bool m_exposureSupported;
    ColorModelSP m_colorModel;
};

typedef KisSharedPtr<EXKoColorConverter> EXColorConverterSP;

#endif





#########################
## EXKoColorConverter.cpp
#########################




#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>

#include "EXKoColorConverter.h"

EXKoColorConverter::EXKoColorConverter(const KoColorSpace *cs)
    : m_colorSpace(cs)
    , m_colorModel(ColorModelFactory::fromKoColorSpace(cs))
{
    const QList<KoChannelInfo *> channelList = cs->channels();

    for (int i = 0; i < channelList.size(); i++) {
        const KoChannelInfo *channel = channelList.at(i);
        quint32 logical = channel->displayPosition();
        m_logicalToMemoryPosition[logical] = i;
    }

    if ((cs->colorDepthId() == Float16BitsColorDepthID || cs->colorDepthId() == Float32BitsColorDepthID
         || cs->colorDepthId() == Float64BitsColorDepthID)
        && cs->colorModelId() != LABAColorModelID && cs->colorModelId() != CMYKAColorModelID) {
        m_exposureSupported = true;
    } else {
        m_exposureSupported = false;
    }
    m_isRGBA = (cs->colorModelId() == RGBAColorModelID);

    const KoColorProfile *profile = cs->profile();
    m_isLinear = (profile && profile->isLinear());

    if (m_isRGBA) {
        m_applyGamma = m_isLinear;
    }
}

KoColor EXKoColorConverter::displayChannelsToKoColor(const QVector4D &channels) const
{
    KoColor c(m_colorSpace);
    QVector<float> channelVec(m_colorSpace->channelCount());
    displayChannelsToKoColor(c.data(), channels, channelVec);
    return c;
}

void EXKoColorConverter::displayChannelsToKoColor(quint8 *target,
                                                const QVector4D &channels,
                                                QVector<float> &tempChannelBuffer) const
{
    QVector4D baseValues(channels);

    for (int i = 0; i < tempChannelBuffer.size(); i++) {
        tempChannelBuffer[m_logicalToMemoryPosition[i]] = baseValues[i];
    }

    m_colorSpace->fromNormalisedChannelsValue(target, tempChannelBuffer);
}

QVector4D EXKoColorConverter::koColorToDisplayChannels(const KoColor &c) const
{
    QVector<float> channelVec(c.colorSpace()->channelCount());
    return koColorToDisplayChannels(c, channelVec);
}

QVector4D EXKoColorConverter::koColorToDisplayChannels(const KoColor &c, QVector<float> &tempChannelBuffer) const
{
    m_colorSpace->normalisedChannelsValue(c.data(), tempChannelBuffer);
    QVector4D channels(0, 0, 0, 0);

    for (int i = 0; i < tempChannelBuffer.size(); i++) {
        channels[i] = tempChannelBuffer[m_logicalToMemoryPosition[i]];
    }

    return channels;
}

const KoColorSpace *EXKoColorConverter::colorSpace() const
{
    return m_colorSpace;
}

const ColorModelSP EXKoColorConverter::colorModel() const
{
    return m_colorModel;
}





