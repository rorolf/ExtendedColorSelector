


#include <QFrame>
#include <QVBoxLayout>
#include <QPainter>
#include <QPaintEvent>
#include <qcolor.h>

#include "EXColorPatchWidget.h"

EXColorPatchWidget::EXColorPatchWidget(QWidget *parent)
    : QWidget(parent)
{
    auto mainLayout = new QVBoxLayout(this);
    setFixedSize(48, 48);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // m_currentColorBox = new QFrame(this);
    // m_lastColorBox = new QFrame(this);
    //
    // mainLayout->addWidget(m_currentColorBox, 1);
    // mainLayout->addWidget(m_lastColorBox, 1);
}

void EXColorPatchWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, false);

    // Fill color
    p.fillRect(rect(), m_color);

    // Selection border
    if (m_selected) {
        QPen pen(Qt::white, 2);
        p.setPen(pen);
        p.drawRect(rect().adjusted(1, 1, -1, -1));
    }
}



void EXColorPatchWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        // Selection should be handled by the parent element
        emit sigClicked();
    }
}

void EXColorPatchWidget::onColorSelected(QColor& color)
{
    m_color = color;
}



