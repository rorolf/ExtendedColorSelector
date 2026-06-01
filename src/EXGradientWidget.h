

#pragma once

#include <QWidget>
#include <QTimer>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <cstdint>
#include <qwidget.h>

#include "EXGradient.h"
#include "EXColorPresetStore.h"


//################################################################################
//## EXGradientPointerWidget
//################################################################################

class EXGradientPointerWidget
{
    public:
        explicit EXGradientPointerWidget(QRectF parentBody, QColor assignedColor=QColor(), float position=-1.0f, float width=16)
        : m_isCurrentPositionPointer(false)
        , m_isSelected(false)
        , m_assignedColor(assignedColor)
        , m_position(position)
        , m_width(width)
        { this->computeBodyShape(parentBody); };
        EXGradientPointerWidget(QRectF parentBody, QColor assignedColor, float position, bool isCurrentPositionPointer)
        : m_isCurrentPositionPointer(isCurrentPositionPointer)
        , m_assignedColor(assignedColor), m_position(position)
        {
            if (isCurrentPositionPointer) { m_width = 12; } else { m_width=16; }
            this->computeBodyShape(parentBody);
        };
        ~EXGradientPointerWidget() {};
        static EXGradientPointerWidget* fromGradient(QRectF parentBody, EXColorGradient &gradient, float position);
        static EXGradientPointerWidget* CurrentPositionPointer(QRectF parentBody);

        float currentPosition() const;
        void changeCurrentPosition(const QRectF container, float position);
        void setSelected(bool selected);

        // QPolygonF's own "contains" method does _not_
        // register events with matching coordinates
        bool contains(const QPointF& point) const;
        void paintSelf(QPainter& painter) const;
        void computeBodyShape(const QRectF& container);

    protected:

    private:

        QPolygonF m_bodyShape;
        bool m_isCurrentPositionPointer = false;
        bool m_isSelected = false;
        QColor m_assignedColor;
        float m_position;
        float m_width;
};

//################################################################################
//## EXGradientPointerContainerWidget
//################################################################################

class EXGradientPointerContainerWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit EXGradientPointerContainerWidget(QWidget* parent);

        int selectedGradientPoint() const;
        float currentPosition() const;

        void setPointsFromGradient(EXColorGradient& gradient);
        void addPointer(const QColor& assignedColor);
        void removePointer();
        void adjustColor(QColor& assignedColor);
        void disable();
        void enable();

    public Q_SLOTS:
        void onGradientPositionChanged(float signal);

    Q_SIGNALS:

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void mousePressEvent(QMouseEvent *event) override;

    private:
        EXGradientPointerWidget* m_currentPositionPointer;
        QVector<EXGradientPointerWidget*> m_pointers;
        int m_selectedPointer = -1;
        bool m_active;
};


//################################################################################
//## EXGradientRectangleWidget
//################################################################################

class EXGradientRectangleWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit EXGradientRectangleWidget(QWidget* parent);
        EXGradientRectangleWidget(QWidget* parent, const EXColorGradient& gradient);

        void setGradient(const EXColorGradient& gradient);
        void disable();
        void enable();

    public Q_SLOTS:

    Q_SIGNALS:

    protected:
        void paintEvent(QPaintEvent* event);
        void resizeEvent(QResizeEvent* event);

    private:
        QVector<QRgb> m_gradientImageData;
        QImage m_gradientImageBase;
        QImage m_gradientImageScaled;

        QTimer m_repaintTimer;
        bool m_sizeChanged = false;
        bool m_active;
};

//################################################################################
//## EXGradientWidget
//################################################################################

class EXGradientWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit EXGradientWidget(QWidget* parent);
        static EXGradientWidget* fromPreset(QWidget* parent, const EXColorPreset& preset, int channelIndex);

        int selectedGradientPoint() const;
        void usePreset(const EXColorPreset& preset, int channelIndex);
        void disable();
        void enable();

    public Q_SLOTS:
        void onGradientSelected(const EXColorPreset& preset, int channelIndex);
        // void onPresetChanged(); // when preset is changed, gradient is deselected
        void onGradientPositionChanged(float signal);

    Q_SIGNALS:
        void sigAddGradientPoint(float position);
        void sigRemoveGradientPoint(int gradientPointIndex);

    protected:

    private:
        EXColorGradient m_gradient;
        int m_channelIndex;
        EXGradientRectangleWidget* m_gradientRectangle;
        EXGradientPointerContainerWidget* m_pointerContainer;

        QDoubleSpinBox* m_tensionSpinbox;
        QDoubleSpinBox* m_continuitySpinBox;
        QDoubleSpinBox* m_biasSpinBox;

        QPushButton* m_addPointerButton;
        QPushButton* m_deletePointerButton;

        // QWidget* m_widgetDisableOverlay;
};












