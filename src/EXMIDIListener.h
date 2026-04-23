#pragma once

#include <QObject>

#include <kis_shared.h>
#include <kis_shared_ptr.h>

#include "EXMIDIEvent.h"
// #include <RtMidi.h>
// #include <libremidi/libremidi.hpp>
// #include "portmidi/portmidi.hpp"
#include <qobject.h>
#include <QRandomGenerator>
// #include "porttime.h" // probably from a separate project
#include <portmidi.h>
// #include "pmutil.h"


class MidiThreadReceiver : public QObject
{
    Q_OBJECT

public:
    explicit MidiThreadReceiver(QObject* parent = nullptr, PmDeviceID openDevice=0)
    : QObject(parent)
    , inputDeviceId(openDevice)
    {
        QRandomGenerator* prng = QRandomGenerator::global();
        this->m_randId = prng->generate();
    };

    ~MidiThreadReceiver();

public Q_SLOTS:
    void start();


Q_SIGNALS:
    void sigMidiMessageArrived(const MidiEvent midiData);
    void sigErrorOccurred(const QString& message);
    void sigFinished();

private:
    PmStream* midiInStream;
    PmDeviceID inputDeviceId;
    quint32 m_randId;
};

class MidiListener : public QObject
{
    Q_OBJECT

public:
    explicit MidiListener(QObject* parent = nullptr);
    ~MidiListener();

    QStringList availableInputPorts() const;

    void startListeningTo(const QString& deviceName);
    void stopListening();

Q_SIGNALS:
    void sigMidiMessageArrived(const MidiEvent midiData);
    void sigErrorOccurred(const QString& message);
    void sigNowListeningTo(const QString& deviceName);

private:
    QThread* midiThread;

};



