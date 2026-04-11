#include "EXMIDILogPanelWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <cmath>

LogPanelWidget::LogPanelWidget(QWidget* parent)
    : QWidget(parent)
{
    QVBoxLayout* logLayout = new QVBoxLayout(this);
    logLayout->setContentsMargins(8, 9, 8, 10);

    QLabel* topSpacing = new QLabel(" ", this);
    logLayout->addWidget(topSpacing);

    // Log view
    logText = new QTextEdit(this);
    logText->setReadOnly(true);
    logLayout->addWidget(logText, 1);

    // Stop Logging checkbox row
    QHBoxLayout* bottomRow = new QHBoxLayout();
    stopLoggingCheckbox = new QCheckBox("Stop Logging", this);
    bottomRow->addWidget(stopLoggingCheckbox);
    bottomRow->addStretch();

    logLayout->addLayout(bottomRow);
}

void LogPanelWidget::appendLine(const QString& line)
{
    if (!stopLoggingCheckbox->isChecked()) {
        logText->append(line);
    }
}

void LogPanelWidget::clearLog()
{
    logText->clear();
}

bool LogPanelWidget::isLoggingEnabled() const
{
    return !stopLoggingCheckbox->isChecked();
}

void LogPanelWidget::onMidiMessage(const MidiEvent& evt) {
    if (this->isLoggingEnabled()) {
        this->appendLine("Event: " + evt.toString());
    }
}
void LogPanelWidget::onKnobTurned(int knob, int value) {
    this->show();
    this->appendLine(QString("→ Knob %1 with value: %2").arg(knob, value));
}
void LogPanelWidget::onPadPressed(int pad, int value) {
    if (std::signbit(value)==0) {
        this->show();
        if (value == 1)
            this->appendLine(QString("→ Pad %1 pressed & state active").arg(pad));
        else
            this->appendLine(QString("→ Pad %1 pressed & state inactive").arg(pad));
    } else {
        this->appendLine(QString("→ Pad %1 pressed but ignored (marked as repeat)").arg(pad));
    }
}





