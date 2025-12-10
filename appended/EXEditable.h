#########################
## EXEditable.h
#########################




#ifndef EXEDITABLEGLIMAGE_H
#define EXEDITABLEGLIMAGE_H

#include <QImage>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPointF>
#include <QVector2D>

#include <KoColorDisplayRendererInterface.h>
#include <KoColorSpace.h>
#include <kis_display_color_converter.h>

#include "KisGLImageF16.h"
#include "KisGLImageWidget.h"

class EXEditableImage : public KisGLImageWidget
{
    Q_OBJECT

public:
    explicit EXEditableImage(QWidget *parent = nullptr);
    ~EXEditableImage() override = default;

    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void paintEvent(QPaintEvent *event) override;
    virtual void startEdit(QMouseEvent *event, bool isShift) = 0;
    virtual void edit(QMouseEvent *event) = 0;
    virtual void shift(QMouseEvent *event, QVector2D delta) = 0;

    void loadQImage(const QImage &image);
    void loadGLImage(const KisGLImageF16 &image);
    void setStretch(bool stretch);
    void setUseGLImage(bool use)
    {
        m_useGLImage = use;
    }
    bool useGLImage() const
    {
        return m_useGLImage;
    }

Q_SIGNALS:
    void sigValueChangeStarted();
    void sigValueChanging();
    void sigValueFinalized();

protected:
    void setDisplayColorConverter(KisDisplayColorConverter *converter);
    KisDisplayColorConverter *displayColorConverter() const;
    KoColorDisplayRendererInterface *displayRenderer() const;
    const KoColorSpace *generationColorSpace(const KoColorSpace *preferredColorSpace = nullptr) const;

private:
    QPointF m_editStart;
    KisDisplayColorConverter *m_displayColorConverter;
    KoColorDisplayRendererInterface *m_displayRenderer;

    QImage m_cachedQImage;
    bool m_useGLImage;
    bool m_stretch;
};

#endif // EXEDITABLEGLIMAGE_H





#########################
## EXEditable.cpp
#########################




#include <QPainter>

#include <KisDisplayConfig.h>
#include <KisSurfaceColorSpaceWrapper.h>
#include <KoColorModelStandardIds.h>
#include <KoColorProfile.h>
#include <KoColorSpace.h>
#include <KoColorSpaceRegistry.h>
#include <kis_display_color_converter.h>

#include "EXEditable.h"

EXEditableImage::EXEditableImage(QWidget *parent)
    : KisGLImageWidget(KisSurfaceColorSpaceWrapper::DefaultColorSpace, parent)
    , m_displayColorConverter(nullptr)
    , m_displayRenderer(nullptr)
    , m_useGLImage(false)
    , m_stretch(true)
{
}

void EXEditableImage::mousePressEvent(QMouseEvent *event)
{
    m_editStart = event->pos();
    startEdit(event, event->modifiers().testFlag(Qt::ShiftModifier) || event->modifiers().testFlag(Qt::AltModifier));

    Q_EMIT sigValueChangeStarted();
}

void EXEditableImage::mouseMoveEvent(QMouseEvent *event)
{
    auto modifiers = event->modifiers();
    float factor = 1.0;
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        factor = 0.1;
    } else if (modifiers.testFlag(Qt::AltModifier)) {
        factor = 0.01;
    }

    if (factor == 1.0f) {
        edit(event);
    } else {
        shift(event, QVector2D(event->pos() - m_editStart) * factor);
    }

    Q_EMIT sigValueChanging();
}

void EXEditableImage::mouseReleaseEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
    Q_EMIT sigValueFinalized();
}

void EXEditableImage::setDisplayColorConverter(KisDisplayColorConverter *converter)
{
    if (m_displayColorConverter == converter) {
        return;
    }

    if (m_displayColorConverter) {
        disconnect(m_displayColorConverter, nullptr, this, nullptr);
    }

    m_displayColorConverter = converter;
    m_displayRenderer = m_displayColorConverter ? m_displayColorConverter->displayRendererInterface() : nullptr;
}

KisDisplayColorConverter *EXEditableImage::displayColorConverter() const
{
    return m_displayColorConverter;
}

KoColorDisplayRendererInterface *EXEditableImage::displayRenderer() const
{
    return m_displayRenderer;
}

const KoColorSpace *EXEditableImage::generationColorSpace(const KoColorSpace *preferredColorSpace) const
{
    KoColorSpaceRegistry *registry = KoColorSpaceRegistry::instance();
    const KoColorProfile *outputProfile =
        m_displayColorConverter ? m_displayColorConverter->openGLCanvasSurfaceDisplayConfig().profile : nullptr;
    const KoColorSpace *outputColorSpace = outputProfile
        ? registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), outputProfile)
        : nullptr;

    const KoColorSpace *result = preferredColorSpace;

    if (!result && m_displayColorConverter) {
        result = m_displayColorConverter->paintingColorSpace();
    }

    if (!result) {
        result = outputColorSpace;
    }

    if (result && result->colorModelId() != RGBAColorModelID) {
        result = outputColorSpace;
    } else if (result && result->colorDepthId() != Float32BitsColorDepthID) {
        result = registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), result->profile());
    }

    if (!result) {
        const KoColorProfile *fallbackProfile = registry->p709SRGBProfile();
        if (fallbackProfile) {
            result = registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), fallbackProfile);
        }
    }

    if (result && result->profile() && registry->p2020PQProfile()
        && result->profile()->uniqueId() == registry->p2020PQProfile()->uniqueId()) {
        const KoColorProfile *g10Profile = registry->p2020G10Profile();
        if (g10Profile) {
            const KoColorSpace *g10Space =
                registry->colorSpace(RGBAColorModelID.id(), Float32BitsColorDepthID.id(), g10Profile);
            if (g10Space) {
                result = g10Space;
            }
        }
    }

    return result;
}

void EXEditableImage::setStretch(bool stretch)
{
    KisGLImageWidget::setStretch(stretch);
    m_stretch = stretch;
}

void EXEditableImage::loadQImage(const QImage &image)
{
    m_useGLImage = false;
    m_cachedQImage = image;
}

void EXEditableImage::loadGLImage(const KisGLImageF16 &image)
{
    m_useGLImage = true;
    KisGLImageWidget::loadImage(image);
}

void EXEditableImage::paintEvent(QPaintEvent *event)
{
    if (m_useGLImage) {
        KisGLImageWidget::paintEvent(event);
    } else {
        QPainter painter(this);
        painter.fillRect(rect(), palette().color(QPalette::Window));
        if (m_stretch) {
            painter.drawImage(rect(), m_cachedQImage);
        } else {
            QSize imageSize = m_cachedQImage.size();
            QPoint center = rect().center();
            QRect drawRect(center.x() - imageSize.width() / 2,
                           center.y() - imageSize.height() / 2,
                           imageSize.width(),
                           imageSize.height());
            painter.drawImage(drawRect, m_cachedQImage);
        }
    }
}





