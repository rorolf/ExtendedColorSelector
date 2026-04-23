#include "EXMIDIListener.h"
#include "EXMIDIEvent.h"
#include <QMetaObject>
#include <QDebug>

#include <QLibrary>
#include <qnamespace.h>
#include <qthread.h>
#include <QDateTime>

//#include <RtMidi.h>
#include <vector>

MidiListener::MidiListener(QObject* parent)
    : QObject(parent)//, midiIn(new RtMidiIn())
    , midiThread(nullptr)
{
    QString deviceName = "";
    this->startListeningTo(deviceName);
}

MidiListener::~MidiListener()
{
    midiThread->quit();
}

QStringList MidiListener::availableInputPorts() const
{
    QStringList ports;
    for (int k = 0; k < Pm_CountDevices(); ++k) {
        const PmDeviceInfo* devInfo = Pm_GetDeviceInfo(k);

        if (!devInfo->output) {
            qDebug() << "PortMidi available input device: " << QString::number(k) << " '" << devInfo->interf << "', '" << devInfo->name << "'";
            ports << QString::fromStdString(devInfo->name);
        } else {
            qDebug() << "PortMidi available output device: '" << devInfo->interf << "', '" << devInfo->name << "'";
        }
    }

    return ports;
}

void MidiListener::startListeningTo(const QString& deviceName) {

    qDebug() << "MidiListener asked to listen to device:" << deviceName;
    PmDeviceID pmidiInputID = Pm_GetDefaultInputDeviceID();
    bool deviceFound = false;

    if (Pm_CountDevices() == 0 || pmidiInputID == pmNoDevice) {
        qDebug() << "Portmidi was unable to find any devices";
    }

    for (int k = 0; k < Pm_CountDevices(); ++k) {
        const PmDeviceInfo* devInfo = Pm_GetDeviceInfo(k);

        if (!devInfo->output) {
            qDebug() << "PortMidi input device:" << QString::number(k) << "'" << devInfo->interf << "', '" << devInfo->name <<"'";
            if (deviceName == devInfo->name) {
                qDebug() << "PortMidi successfully found the requested Device";
                deviceFound = true;
                pmidiInputID = static_cast<PmDeviceID>(k);
            }
        } else if (deviceName == devInfo->name) {
            qDebug() << "PortMidi found requested input device, but it is an output device.";
        } else {
            qDebug() << "PortMidi output device:" << QString::number(k) << "'" << devInfo->interf << "', '" << devInfo->name<<"'";
        }
    }

    if (deviceFound && (this->midiThread != nullptr)) {
        qDebug() << "MidiListener quits its thread.";
        this->midiThread->quit();
        this->midiThread = nullptr;
    }
    if (!(this->midiThread)) {
        qDebug() << "MidiListener starts a new thread.";
        this->midiThread = new QThread(this);
        qDebug() << "MidiListener creates a new MidiThreadReceiver.";
        MidiThreadReceiver* midiReceiver = new MidiThreadReceiver(nullptr, pmidiInputID);
         // on same thread
        connect(midiThread, &QThread::started, midiReceiver, [midiReceiver](){ midiReceiver->start(); });
        // between threads
        connect(midiReceiver, &MidiThreadReceiver::sigMidiMessageArrived, this, &MidiListener::sigMidiMessageArrived, Qt::QueuedConnection);
        // connect(midiReceiver, &MidiThreadReceiver::sigMidiMessageArrived, this, [this](MidiEvent evt) {
        //     qDebug() << "MidiListener received event:" << evt.toString();
        //     emit this->sigMidiMessageArrived(evt);
        // }, Qt::QueuedConnection);
        connect(midiReceiver, &MidiThreadReceiver::sigErrorOccurred, this, &MidiListener::sigErrorOccurred, Qt::QueuedConnection);
        qDebug() << "MidiListener moves the new MidiThreadReceiver to a new thread.";
        midiReceiver->moveToThread(midiThread);
        midiThread->start(QThread::TimeCriticalPriority);

        emit sigNowListeningTo(deviceName);
    }
}

void MidiListener::stopListening() {
    if (this->midiThread) {
        qDebug() << "MidiListener stops listening.";
        midiThread->quit();
        midiThread = nullptr;
    } else { qDebug() << "MidiListener had already stopped listening."; }
}

MidiThreadReceiver::~MidiThreadReceiver()
{
    qDebug() << "MidiThreadReceiver gets destroyed.";
    Pm_Close(midiInStream);
}

void MidiThreadReceiver::start() {

    qDebug() << "MidiThreadReceiver starts operating.";
    void* DRIVER_INFO = nullptr;
    int32_t INPUT_BUFFER_SIZE = 1024;
    PmTimeProcPtr TIME_PROC = nullptr; // originally: ((PmTimeProcPtr) Pt_Time);
    void* TIME_INFO = nullptr;
    qDebug() << "MidiThreadReceiver opens stream.";
    // TODO: How to detect Failure?
    Pm_OpenInput(&midiInStream, this->inputDeviceId, DRIVER_INFO, INPUT_BUFFER_SIZE,
                     TIME_PROC, TIME_INFO);

    QThread::msleep(500);

    PmEvent midiEventBuffer[1];
    int msgLength;
    while (true) {
        PmError pollStatus = Pm_Poll(midiInStream);

        if (pollStatus == true) {
            msgLength = Pm_Read(midiInStream, midiEventBuffer, 1);
            if (msgLength > 0) {
                MidiEvent midiData = MidiEvent::fromPortMidiMessage(midiEventBuffer[0]);
                // qDebug() << "PortMidi received message:" << midiData.toString();
                emit sigMidiMessageArrived(midiData);
            }
        }
        QThread::msleep(5);
    }
}


















