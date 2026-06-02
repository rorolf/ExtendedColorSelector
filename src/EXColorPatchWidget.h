
#ifndef EXCOLORPATCHWIDGET_H
#define EXCOLORPATCHWIDGET_H


#include <QColor>
#include <QDialog>
#include <QFrame>
#include <QWidget>
#include <qvector3d.h>
#include <QPainter>
#include <QPaintEvent>

#include "EXGradient.h"

class EXColorPatchWidget : public QWidget
{
    Q_OBJECT
public:
    explicit EXColorPatchWidget(QWidget *parent = nullptr);

    QColor m_color;
    bool m_selected = false;
    bool m_depictsGradient = false;

    void loadColorFromRaw(QColor newClr);
    void loadColorFromGradient(const EXColorGradient& gradient);

Q_SIGNALS:
    void sigClicked();
    void sigIsSelected(bool isSelected);

public Q_SLOTS:
    void onColorSelected(QColor& color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:

};

#endif // EXCOLORPATCHWIDGET
