#pragma once

#include <QObject>
#include <RtMidi.h>
#include "MappingEntry.h"

class MidiListener : public QObject
{
    Q_OBJECT

public:
    explicit MidiListener(QObject* parent = nullptr);
    ~MidiListener();

    QStringList availableInputPorts() const;
    void openPort(int index);
    void closePort();

signals:
    void midiMessageReceived(const QByteArray& data);
    void errorOccurred(const QString& message);

private:
    RtMidiIn* midiIn;
    static void midiCallback(double, std::vector<unsigned char>*, void* userData);

    QList<MappingEntry> mappings;
};
