#########################
## OptionalColorPicker.h
#########################




#ifndef OPTIONALCOLORPICKER_H
#define OPTIONALCOLORPICKER_H

#include <QCheckBox>
#include <QColorDialog>
#include <QPushButton>
#include <QWidget>

class OptionalColorPicker : public QWidget
{
    Q_OBJECT

public:
    OptionalColorPicker(QWidget *parent, const QString &labelText, const QColor &defaultColor);
    ~OptionalColorPicker() override = default;
    void setPickingEnabled(bool enabled);

    QColorDialog *colorDialog;
    QColor cachedColor;
    QCheckBox *enableBox;
    QPushButton *indicator;
};

#endif // OPTIONALCOLORPICKER_H





#########################
## OptionalColorPicker.cpp
#########################




#include <QHBoxLayout>

#include "OptionalColorPicker.h"

OptionalColorPicker::OptionalColorPicker(QWidget *parent, const QString &labelText, const QColor &defaultColor)
    : QWidget(parent)
    , colorDialog(new QColorDialog(this))
    , cachedColor(defaultColor)
    , enableBox(new QCheckBox(labelText, this))
    , indicator(new QPushButton(this))
{
    auto mainLayout = new QHBoxLayout(this);
    setLayout(mainLayout);

    mainLayout->addWidget(enableBox);
    mainLayout->addWidget(indicator);

    indicator->setFocusPolicy(Qt::NoFocus);

    connect(indicator, &QPushButton::clicked, this, [this]() {
        colorDialog->exec();
    });

    auto updateColor = [this](const QColor &color) {
        cachedColor = color;
        indicator->setStyleSheet(QString("QPushButton { background-color: %1; border: none; }").arg(color.name()));
    };
    updateColor(defaultColor);
    connect(colorDialog, &QColorDialog::colorSelected, this, updateColor);
}

void OptionalColorPicker::setPickingEnabled(bool enabled)
{
    if (enabled) {
        indicator->show();
    } else {
        indicator->hide();
    }
    enableBox->setChecked(enabled);
}





