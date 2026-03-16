#pragma once

#include "EXMIDIEvent.h"
#include <QWidget>
#include <QTextEdit>
#include <QCheckBox>

class LogPanelWidget : public QWidget
{
    Q_OBJECT

public:
    explicit LogPanelWidget(QWidget* parent = nullptr);

    void appendLine(const QString& line);
    void clearLog();
    bool isLoggingEnabled() const;

public Q_SLOTS:
    void onMidiMessage(const MidiEvent& evt);
    void onKnobTurned(int knob, int value);
    void onPadPressed(int pad, int value);

private:
    QTextEdit* logText;
    QCheckBox* stopLoggingCheckbox;
};
