#pragma once

#include <QObject>
#include "RtMidi.h"
#include "EXMIDIMappingEntry.h"

class MidiListener : public QObject
{
    Q_OBJECT

public:
    explicit MidiListener(QObject* parent = nullptr);
    ~MidiListener();

    QStringList availableInputPorts() const;
    void openPort(int index);
    void closePort();

Q_SIGNALS:
    void sigMidiMessageReceived(const QByteArray& data);
    void sigErrorOccurred(const QString& message);

private:
    RtMidiIn* midiIn;
    static void midiCallback(double, std::vector<unsigned char>*, void* userData);

    QList<MappingEntry> mappings;
};
