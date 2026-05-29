

#include "EXGradientWidget.h"

#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <qboxlayout.h>
#include <qcolor.h>
#include <qvector3d.h>
#include <qwidget.h>
#include <algorithm>

#include "EXColorMixState.h"


//################################################################################
//## EXGradientPointerWidget
//################################################################################


EXGradientPointerWidget* EXGradientPointerWidget::fromGradient(EXColorGradient &gradient, float position)
{
    QVector3D assignedColorRep = gradient.colorAt(position);
    QColor assignedColor = EXColorMixState::instance()->toQColor(assignedColorRep);
    return new EXGradientPointerWidget(assignedColor, position, false);
};

EXGradientPointerWidget* EXGradientPointerWidget::CurrentPositionPointer() {
    return new EXGradientPointerWidget(QColor(), 0.5f, true);
}

float EXGradientPointerWidget::currentPosition() const {
    return m_position;
}

void EXGradientPointerWidget::computeBodyShape(const QRectF& container) {

    float midPointPosition = m_position;
    qreal offset = container.left() + std::clamp(midPointPosition, 0.0f, 1.0f)*container.width() - m_width/2;
    // determine bounding box
    QRectF sizeRect = QRectF(offset, container.top(), m_width, container.height());
    QPointF tL = sizeRect.topLeft();
    QPointF tR = sizeRect.topRight();
    QPointF bL = sizeRect.bottomLeft();
    QPointF bR = sizeRect.bottomRight();

    QPointF p1 = bL;
    QPointF p2 = (bL + tL)/2;
    QPointF p3 = (tL+tR)/2;
    QPointF p4 = (bR + tR)/2;
    QPointF p5 = bR;

    //truncate shape at the border
    qreal left = container.left(); qreal right = container.right();
    if (p1.x()<left) p1.setX(left);
    if (p2.x()<left) p2.setX(left);
    if (p4.x()>right) p4.setX(right);
    if (p5.x()>right) p5.setX(right);

    this->m_bodyShape = QPolygonF({p1, p2, p3, p4, p5});
}

// QPolygonF's contains does _not_ accept points with correct coordinates
bool EXGradientPointerWidget::contains(const QPointF& point) const {
    qDebug() << QString("contains was called with: (%1, %2)").arg(point.x()).arg(point.y());
    qDebug() << QString("body size: %1").arg(m_bodyShape.size());
    for (int k=0; k<m_bodyShape.size(); ++k) {
        qDebug() << QString("\tbody point: (%1,%2)").arg(m_bodyShape.at(k).x()).arg(m_bodyShape.at(k).y());
    }
    if (m_bodyShape.isEmpty()) return false;

    // QPointF bL = m_bodyShape.at(0); // unused
    QPointF tL = m_bodyShape.at(1);
    QPointF tP = m_bodyShape.at(2);
    QPointF tR = m_bodyShape.at(3);
    QPointF bR = m_bodyShape.at(4);


    float eX = point.x(); float eY = point.y();
    // bottom rectangle bLbRtRtL
    if ((eX>=tL.x()) && (eY>=tL.y()) &&
        (eX<=bR.x()) && (eY<=bR.y())) return true;

    // top triangle tLtRtP is always based on an x-axis parallel segment tL <> tR
    if ((eY<tL.y()) || eY>tP.y()) return false;
    // and left and right are known, so there is A left and B right of (eX,eY) iff (eX,eY) is inside the triangle
    // A = tL + α*(tP-tL) && tP = tR + β*(tP-tR), and A.y() = B.y() = eY
    // eY -tL.y() = α*(tP-tL).y()
    float α = (eY - tL.y()) / (tP.y() - tL.y());
    float β = (eY - tR.y()) / (tP.y() - tR.y());
    float Ax = tL.x() + α*(tP.x() - tL.x());
    float Bx = tR.x() + β*(tP.x() - tR.x());

    if ((Ax<=eX) && (eX<=Bx)) return true;
    else return false;
}

void EXGradientPointerWidget::paintSelf(QPainter& p) const {
    p.setRenderHint(QPainter::Antialiasing, false);
    if (m_isCurrentPositionPointer) {
        if (m_isSelected) { p.setPen(QColor(200,200,255)); }
        else { p.setPen(QColor(0,0,153)); }

        p.setBrush(QColor(0,0,0));
    } else {
        if (m_isSelected) {
            if (m_assignedColor.lightness() > 120) { p.setPen(QColor(0,0,0)); }
            else { p.setPen(QColor(240, 240, 255)); }
        } else {
            p.setPen(m_assignedColor);
        }
        p.setBrush(m_assignedColor);
    }

    p.drawConvexPolygon(m_bodyShape);
}

void EXGradientPointerWidget::changeCurrentPosition(const QRectF container, float position) {
    m_position = std::clamp(position, 0.0f, 1.0f);
    this->computeBodyShape(container);
};

void EXGradientPointerWidget::setSelected(bool selected) {
    m_isSelected = selected;
}

//################################################################################
//## EXGradientPointerContainerWidget
//################################################################################

EXGradientPointerContainerWidget::EXGradientPointerContainerWidget(QWidget* parent)
: QWidget(parent)
, m_currentPositionPointer(EXGradientPointerWidget::CurrentPositionPointer())
, m_pointers({ new EXGradientPointerWidget(QColor(), 0.0f), new EXGradientPointerWidget(QColor(255,255,255), 1.0f) })
{ }

void EXGradientPointerContainerWidget::setPointsFromGradient(EXColorGradient& gradient) {
    m_selectedPointer = -1;
    for (EXGradientPointerWidget* pointer : m_pointers) { delete pointer; }
    m_pointers.clear();
    const QVector<EXGradientColor>& interpolPoints = gradient.m_colorSpline.points();
    for (int k=0; k<interpolPoints.size(); ++k) {
        float pos = interpolPoints[k].m_positionOnGradient;
        EXGradientPointerWidget* pointer = EXGradientPointerWidget::fromGradient(gradient, pos);
        m_pointers.push_back(pointer);
    }

    this->update();
}

int EXGradientPointerContainerWidget::selectedGradientPoint() const {
    return m_selectedPointer;
}

void EXGradientPointerContainerWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
    QPainter p(this);
    p.setPen(QColor(0,64,64));
    p.setBrush(QColor(0,220,220));
    p.drawRect(this->rect().adjusted(1, 1, -1, -1));

    for (EXGradientPointerWidget* pointer : m_pointers) { pointer->paintSelf(p); }
    m_currentPositionPointer->paintSelf(p);
}

void EXGradientPointerContainerWidget::resizeEvent(QResizeEvent *event) {
    QRectF rect = this->rect();
    for (EXGradientPointerWidget* pointer : m_pointers) { pointer->computeBodyShape(rect); }
    m_currentPositionPointer->computeBodyShape(rect);

    QWidget::resizeEvent(event);
}



void EXGradientPointerContainerWidget::mousePressEvent(QMouseEvent *event)
{
    QPointF queryPos = event->localPos();
    if (m_currentPositionPointer->contains(queryPos)) {
        event->ignore(); return;
    }

    int clickedPointer = -2;
    for (int k=0; k<m_pointers.size(); ++k) {
        if (m_pointers[k]->contains(queryPos)) clickedPointer = k;
    }
    qDebug() << QString("Hit event calculations ended with: %1").arg(clickedPointer);
    if (clickedPointer < 0) {
        if (m_selectedPointer >= 0) { m_pointers[m_selectedPointer]->setSelected(false); }
        m_selectedPointer = -1;
    } else {
        m_selectedPointer = clickedPointer;
        m_pointers[m_selectedPointer]->setSelected(true);
    }
    this->update();
}

void EXGradientPointerContainerWidget::onGradientPositionChanged(float signal) {
    qDebug() << QString("GradientPointerContainer received signal: %1").arg(signal);
    if (m_selectedPointer>=0) { m_pointers[m_selectedPointer]->changeCurrentPosition(this->rect(), signal); }
    else { m_currentPositionPointer->changeCurrentPosition(this->rect(), signal); }
    this->update();
};


void EXGradientPointerContainerWidget::addPointer(const QColor& assignedColor) {
    float position = m_currentPositionPointer->currentPosition();
     EXGradientPointerWidget* newpointer = new EXGradientPointerWidget(assignedColor, position);
     m_pointers.push_back(newpointer);
};

void EXGradientPointerContainerWidget::removePointer() {
    if ( (m_selectedPointer>=0) && (m_selectedPointer<m_pointers.size()) ) {
        EXGradientPointerWidget* rmWidget = m_pointers[m_selectedPointer];
        m_pointers.remove(m_selectedPointer);
        m_selectedPointer = -1;
        delete rmWidget;
    }
};


//################################################################################
//## EXGradientRectangleWidget
//################################################################################

EXGradientRectangleWidget::EXGradientRectangleWidget(QWidget* parent)
: QWidget(parent)
{
    this->m_repaintTimer.setInterval(100);
    connect(&m_repaintTimer, &QTimer::timeout, this, [this]() {
        if (!m_gradientImageBase.isNull() && m_sizeChanged) {
            m_gradientImageScaled = m_gradientImageBase.scaled(this->width(), this->height());
            m_sizeChanged = false;
        }
    });
    this->m_repaintTimer.start();
}

EXGradientRectangleWidget::EXGradientRectangleWidget(QWidget* parent, const EXColorGradient& gradient)
: QWidget(parent)
{
    this->setGradient(gradient);

    this->m_repaintTimer.setInterval(100);
    connect(&m_repaintTimer, &QTimer::timeout, this, [this]() {
        if (!m_gradientImageBase.isNull() && m_sizeChanged) {
            m_gradientImageScaled = m_gradientImageBase.scaled(this->width(), this->height());
            m_sizeChanged = false;
        }
    });
    this->m_repaintTimer.start();
}

void EXGradientRectangleWidget::setGradient(const EXColorGradient& colorGradient) {
    if (m_gradientImageData.size() != 128*4) { //AA RR GG BB
        m_gradientImageData.clear(); // may unreserve memory
        for (int k=0; k<128*4; ++k) { m_gradientImageData.push_back(QRgb()); }
    }
    EXColorMixState* mixer = EXColorMixState::instance();
    for (int k=0; k<128; ++k) {
        float pos = float(k)/128.0f;
        QVector3D repr = colorGradient.colorAt(pos);
        QRgb clr = mixer->toQColor(repr).rgb();
        m_gradientImageData[k] = clr;
    }
    const uchar* rawData = reinterpret_cast<uchar*>(m_gradientImageData.data());
    m_gradientImageBase = QImage(rawData, 128, 1, QImage::Format_ARGB32);
    m_sizeChanged = true;
    this->update();
}

void EXGradientRectangleWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    m_sizeChanged = true;
}

void EXGradientRectangleWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    if (!m_gradientImageScaled.isNull()) {
        p.drawImage(0, 0, m_gradientImageScaled);
    } else {
        p.setBrush(QColor(128,0,128));
    }
    p.setPen(QColor(128,128,0));
    p.drawRect(this->rect().adjusted(1, 1, -1, -1));
}

//################################################################################
//## EXGradientWidget
//################################################################################

EXGradientWidget::EXGradientWidget(QWidget* parent)
: QWidget(parent)
{
    this->m_gradient = EXColorGradient();

    QHBoxLayout* mainLayout = new QHBoxLayout(this);

    QVBoxLayout* gradientLayout = new QVBoxLayout(this);
    this->m_gradientRectangle = new EXGradientRectangleWidget(this);
    m_gradientRectangle->setGradient(m_gradient);
    this->m_pointerContainer = new EXGradientPointerContainerWidget(this);
    gradientLayout->addWidget(m_gradientRectangle);
    gradientLayout->addWidget(m_pointerContainer);

    QVBoxLayout* parameterLayout = new QVBoxLayout(this);
    this->m_tensionSpinbox = new QDoubleSpinBox(this);
    this->m_continuitySpinBox = new QDoubleSpinBox(this);
    this->m_biasSpinBox = new QDoubleSpinBox(this);
    for (QDoubleSpinBox* spinBox : {m_tensionSpinbox, m_continuitySpinBox, m_biasSpinBox}) {
        spinBox->setRange(-1, 1);
        spinBox->setSingleStep(0.05);
        parameterLayout->addWidget(spinBox);
    }

    QVBoxLayout* buttonLayout = new QVBoxLayout(this);
    this->m_addPointerButton = new QPushButton(this);
    m_addPointerButton->setText("+");
    this->m_deletePointerButton = new QPushButton(this);
    m_deletePointerButton->setText("x");
    buttonLayout->addWidget(m_addPointerButton);
    buttonLayout->addWidget(m_deletePointerButton);

    mainLayout->addLayout(gradientLayout);  mainLayout->setStretchFactor(gradientLayout,  3);
    mainLayout->addLayout(parameterLayout); mainLayout->setStretchFactor(parameterLayout, 1);
    mainLayout->addLayout(buttonLayout);    mainLayout->setStretchFactor(buttonLayout,    1);
};


EXGradientWidget* EXGradientWidget::fromPreset(QWidget* parent, const EXColorPreset& preset, int channelIndex)
{
    EXGradientWidget* widget = new EXGradientWidget(parent);
    widget->usePreset(preset, channelIndex);

    return widget;
};

int EXGradientWidget::selectedGradientPoint() const {
    return m_pointerContainer->selectedGradientPoint();
}

void EXGradientWidget::usePreset(const EXColorPreset& preset, int channelIndex) {
    if ((channelIndex<0) || (channelIndex>=preset.m_mixGradients.size())) { return; }

    m_gradient = preset.m_mixGradients[channelIndex];
    m_gradientRectangle->setGradient(this->m_gradient);
    m_pointerContainer->setPointsFromGradient(this->m_gradient);
};

void EXGradientWidget::onGradientSelected(const EXColorPreset& preset, int channelIndex) {
    this->usePreset(preset,channelIndex);
};

void EXGradientWidget::onGradientPositionChanged(float signal) {
    m_pointerContainer->onGradientPositionChanged(signal);
};


















