

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

#include "EXColorMixState.h"


//################################################################################
//## EXGradientPointerWidget
//################################################################################


EXGradientPointerWidget* EXGradientPointerWidget::fromGradient(EXColorGradient &gradient, float position)
{
    return EXGradientPointerWidget::fromGradient(nullptr, gradient, position);
};

EXGradientPointerWidget* EXGradientPointerWidget::fromGradient(QWidget* parent, EXColorGradient &gradient, float position)
{
    QVector3D assignedColorRep = gradient.colorAt(position);
    QColor assignedColor = EXColorMixState::instance()->toQColor(assignedColorRep);
    return new EXGradientPointerWidget(parent, assignedColor, position, false);
};

EXGradientPointerWidget* EXGradientPointerWidget::CurrentPositionPointer(QWidget* parent) {
    return new EXGradientPointerWidget(parent, QColor(), 0.0f, true);
}

float EXGradientPointerWidget::currentPosition() {
    return m_position;
}

void EXGradientPointerWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);
    if (m_isCurrentPositionPointer) {
        p.setPen(QColor(0,0,153));
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

    QRect sizeRect = this->rect();
    QPoint tL = sizeRect.topLeft();
    QPoint tR = sizeRect.topRight();
    QPoint bL = sizeRect.bottomLeft();
    QPoint bR = sizeRect.bottomRight();

    QPoint p1 = bL; QPoint p2 = (bL + tL)/2;
    QPoint p3 = (tL+tR)/2;
    QPoint p4 = (bR + tR)/2;
    QPoint p5 = bR;

    QPolygonF shape = QPolygonF({p1, p2, p3, p4, p5});
    p.drawConvexPolygon(shape);
}



void EXGradientPointerWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Selection should be handled by the parent element
        emit sigClicked();
    }
}

void EXGradientPointerWidget::onAssignedDataChanged(QColor newClr, float position) {
    m_assignedColor = newClr; m_position = position;
};

//################################################################################
//## EXGradientPointerContainerWidget
//################################################################################

EXGradientPointerContainerWidget::EXGradientPointerContainerWidget(QWidget* parent)
: QWidget(parent)
, m_currentPositionPointer(EXGradientPointerWidget::CurrentPositionPointer(this))
, m_pointers({ new EXGradientPointerWidget(this), new EXGradientPointerWidget(this) })
{ }

void EXGradientPointerContainerWidget::setPointsFromGradient(EXColorGradient& gradient) {
    m_pointers.clear();
    const QVector<EXGradientColor>& interpolPoints = gradient.m_colorSpline.points();
    for (int k=0; k<interpolPoints.size(); ++k) {
        float pos = interpolPoints[k].m_positionOnGradient;
        EXGradientPointerWidget* pointer = EXGradientPointerWidget::fromGradient(gradient, pos);
        m_pointers.push_back(pointer);
    }
    this->update();
}

void EXGradientPointerContainerWidget::paintEvent(QPaintEvent *event) {
    QWidget::paintEvent(event);
}

void EXGradientPointerContainerWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    int parentWidth = this->width();

    for (EXGradientPointerWidget* pointer : m_pointers) {
        float pointerX = static_cast<int>(pointer->currentPosition()*float(parentWidth));
        pointer->move(pointerX, 0);
    }
}


void EXGradientPointerContainerWidget::addPointer(const QColor& assignedColor) {
    float position = m_currentPositionPointer->currentPosition();
     EXGradientPointerWidget* newpointer = new EXGradientPointerWidget(this, assignedColor, position);
     m_pointers.push_back(newpointer);
};

void EXGradientPointerContainerWidget::removePointer() {
    if ( (m_selectedPointer>=0) && (m_selectedPointer<m_pointers.size()) ) {
        EXGradientPointerWidget* rmWidget = m_pointers[m_selectedPointer];
        m_pointers.remove(m_selectedPointer);
        m_selectedPointer = -1;
        rmWidget->deleteLater();
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
        spinBox->setSingleStep(0.1);
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


EXGradientWidget* EXGradientWidget::fromPreset(QWidget* parent, EXColorPreset* preset, int channelIndex)
{
    EXGradientWidget* widget = new EXGradientWidget(parent);
    widget->m_gradient = preset->m_mixGradients[channelIndex];
    widget->m_gradientRectangle->setGradient(widget->m_gradient);
    widget->m_pointerContainer->setPointsFromGradient(widget->m_gradient);

    return widget;
};

void EXGradientWidget::onGradientSelected(int channelIndex) {};
        // void onPresetChanged(); // when preset is changed, gradient is deselected
void EXGradientWidget::onGradientPositionChanged(float signal) { };
void EXGradientWidget::onAddGradientPointer() {};
void EXGradientWidget::onGradientPointerSelected(EXGradientPointerWidget* pointerWidget) {};
void EXGradientWidget::onRemoveGradientPointer(EXGradientPointerWidget* pointerWidget) {};















