#pragma once

#include <QObject>

#include <kis_shared.h>
#include <kis_shared_ptr.h>

#include "EXMIDIEvent.h"
// #include <RtMidi.h>
// #include <libremidi/libremidi.hpp>
// #include "portmidi/portmidi.hpp"
#include <qobject.h>
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
    {};
    ~MidiThreadReceiver();

public Q_SLOTS:
    void start();


Q_SIGNALS:
    void sigMidiMessageArrived(const MidiEvent midiData);
    void sigErrorOccurred(const QString& message);

private:
    PmStream* midiInStream;
    PmDeviceID inputDeviceId;
};

class MidiListener : public QObject
{
    Q_OBJECT

public:
    explicit MidiListener(QObject* parent = nullptr);
    ~MidiListener();

    QThread* midiThread;

    QStringList availableInputPorts() const;
    void openPort(int index);
    void closePort();

    void startOrReplaceMidiReceiver(const QString& deviceName);

Q_SIGNALS:
    void sigMidiMessageArrived(const MidiEvent midiData);
    void sigErrorOccurred(const QString& message);

private:
    // RtMidiIn* midiIn;
    // libremidi::midi_out
    static void rtMidiCallback(double, std::vector<unsigned char>*, void* userData);
};

//typedef KisSharedPtr<MidiListener> MidiListenerSP;



