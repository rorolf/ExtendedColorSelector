

#include <QWidget>
#include <QTimer>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <qwidget.h>

#include "EXGradient.h"
#include "EXColorPresetStore.h"


//################################################################################
//## EXGradientPointerWidget
//################################################################################

class EXGradientPointerWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit EXGradientPointerWidget(QWidget* parent = nullptr, QColor assignedColor=QColor(), float position=-1.0f)
        : QWidget(parent)
        , m_isCurrentPositionPointer(false)
        , m_isSelected(false)
        , m_assignedColor(assignedColor)
        , m_position(position)
        {};
        EXGradientPointerWidget(QWidget* parent, QColor assignedColor, float position, bool isCurrentPositionPointer)
        : QWidget(parent), m_isCurrentPositionPointer(isCurrentPositionPointer)
        , m_assignedColor(assignedColor), m_position(position) {};
        ~EXGradientPointerWidget() {};
        static EXGradientPointerWidget* fromGradient(EXColorGradient &gradient, float position);
        static EXGradientPointerWidget* fromGradient(QWidget* parent, EXColorGradient &gradient, float position);
        static EXGradientPointerWidget* CurrentPositionPointer(QWidget* parent);

        float currentPosition();
    public Q_SLOTS:
        void onAssignedDataChanged(QColor newClr, float position);

    Q_SIGNALS:
        void sigClicked();

    protected:
        void paintEvent(QPaintEvent* event) override;
        void mousePressEvent(QMouseEvent *event) override;

    private:
        bool m_isCurrentPositionPointer = false;
        bool m_isSelected = false;
        QColor m_assignedColor;
        float m_position;
};

//################################################################################
//## EXGradientPointerContainerWidget
//################################################################################

class EXGradientPointerContainerWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit EXGradientPointerContainerWidget(QWidget* parent);

        void setPointsFromGradient(EXColorGradient& gradient);
        void addPointer(const QColor& assignedColor);
        void removePointer();
        void adjustColor(QColor& assignedColor);

    public Q_SLOTS:
        void onGradientPositionChanged(float signal) {};
        void onPointerSelected(EXGradientPointerWidget* pointer) {};


    Q_SIGNALS:

    protected:
        void paintEvent(QPaintEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        EXGradientPointerWidget* m_currentPositionPointer;
        QVector<EXGradientPointerWidget*> m_pointers;
        int m_selectedPointer = -1;
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
};

//################################################################################
//## EXGradientWidget
//################################################################################

class EXGradientWidget : public QWidget
{
    Q_OBJECT

    public:
        explicit EXGradientWidget(QWidget* parent);
        static EXGradientWidget* fromPreset(QWidget* parent, EXColorPreset* preset, int channelIndex);

    public Q_SLOTS:
        void onGradientSelected(int channelIndex);
        // void onPresetChanged(); // when preset is changed, gradient is deselected
        void onGradientPositionChanged(float signal);
        void onAddGradientPointer();
        void onGradientPointerSelected(EXGradientPointerWidget* pointerWidget);
        void onRemoveGradientPointer(EXGradientPointerWidget* pointerWidget);

    Q_SIGNALS:

    protected:

    private:
        EXColorGradient m_gradient;
        EXGradientRectangleWidget* m_gradientRectangle;
        EXGradientPointerContainerWidget* m_pointerContainer;

        QDoubleSpinBox* m_tensionSpinbox;
        QDoubleSpinBox* m_continuitySpinBox;
        QDoubleSpinBox* m_biasSpinBox;

        QPushButton* m_addPointerButton;
        QPushButton* m_deletePointerButton;
};












