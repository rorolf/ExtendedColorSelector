#include "EXMIDIListener.h"
#include "EXMIDIEvent.h"
#include <QMetaObject>
#include <QDebug>

#include <QLibrary>
#include <qthread.h>
#include <QDateTime>

//#include <RtMidi.h>
#include <vector>

MidiListener::MidiListener(QObject* parent)
    : QObject(parent)//, midiIn(new RtMidiIn())
    , midiThread(nullptr)
{

    //################################################################################
    //##  PortMidi Test
    //################################################################################

    QString deviceName = "LPD8 mk2 MIDI 1";
    this->startOrReplaceMidiReceiver(deviceName);


    //################################################################################
    //## RtMidi Test
    //################################################################################

    // QLibrary myLib("mylib");
    // typedef void (*MyPrototype)();
    // MyPrototype myFunction = (MyPrototype) myLib.resolve("mysymbol");
    // if (myFunction)
    //     myFunction();

    // QLibrary rtMidiLib("librtmidi.so");
    // typedef RtMidiIn* (*MidiF)();
    // MidiF getRtMidiIn = (MidiF)rtMidiLib.resolve("rtmidi_in_create");
    //
    // if (getRtMidiIn) {
    //     //midiIn = new RtMidiIn();
    //     midiIn = getRtMidiIn();
    // } else {
    //     qDebug() << "Failed to load RtMidi dynamically";
    //     throw std::runtime_error("Failed to load RtMidi dynamically");
    // }

    // midiIn = new RtMidiIn();
    //
    // std::vector<RtMidi::Api> apis;
    // RtMidi::getCompiledApi(apis);
    // qDebug() << "Available APIs:";
    // for (auto api : apis) {
    //     qDebug() << QString::fromStdString(RtMidi::getApiName(api));
    // }
    // qDebug() << "    Current API:" << QString::fromStdString(RtMidi::getApiName(midiIn->getCurrentApi()));


    // Do not ignore sysex messages
    // Do not ignore timing messages
    // Do not ignore active sensing messages (continuous stream of current state)
    // midiIn->ignoreTypes(false, false, false);
}

MidiListener::~MidiListener()
{
    closePort();
    // delete midiIn;
    midiThread->quit();
}

QStringList MidiListener::availableInputPorts() const
{
    // QStringList ports;
    // unsigned int nPorts = midiIn->getPortCount();
    // for (unsigned int i = 0; i < nPorts; ++i) {
    //     qDebug() << QString::fromStdString(midiIn->getPortName(i));
    //     ports << QString::fromStdString(midiIn->getPortName(i));
    // }
    // qDebug() << "All Ports collected";

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

void MidiListener::openPort(int index)
{
    Q_UNUSED(index);
    // closePort();  // Close any existing port
    // try {
    //     midiIn->openPort(index);
    //     midiIn->setCallback(&MidiListener::midiCallback, this);
    // } catch (RtMidiError& e) {
    //     emit sigErrorOccurred(QString::fromStdString(e.getMessage()));
    // }
}

void MidiListener::closePort()
{
    // if (midiIn->isPortOpen()) {
    //     midiIn->closePort();
    // }
}

void MidiListener::rtMidiCallback(double, std::vector<unsigned char>* message, void* userData)
{
    auto* self = static_cast<MidiListener*>(userData);
    if (!self) return;

    QByteArray messageData(reinterpret_cast<const char*>(message->data()), int(message->size()));
    if (messageData.size() < 3) return;

    MidiEvent midiData = MidiEvent::fromRtMidiMessage(messageData);

    if (midiData.type != MidiEventType::Unknown) {
        QMetaObject::invokeMethod(self, "sigMidiMessageArrived",
                              Qt::QueuedConnection,
                              Q_ARG(MidiEvent, midiData));
    }
}

void MidiListener::startOrReplaceMidiReceiver(const QString& deviceName) {
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
        connect(midiReceiver, &MidiThreadReceiver::sigErrorOccurred, this, &MidiListener::sigErrorOccurred, Qt::QueuedConnection);
        qDebug() << "MidiListener moves the new MidiThreadReceiver to a new thread.";
        midiReceiver->moveToThread(midiThread);
        midiThread->start(QThread::TimeCriticalPriority);
    }
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
    // #define DEVICE_INFO NULL
    // #define TIME_PROC ((PmTimeProcPtr) Pt_Time)
    void* TIME_INFO = nullptr;
    qDebug() << "MidiThreadReceiver opens stream.";
    Pm_OpenInput(&midiInStream, this->inputDeviceId, DRIVER_INFO, INPUT_BUFFER_SIZE,
                     TIME_PROC, TIME_INFO);

    QThread::msleep(500);

    PmEvent midiEventBuffer[1];
    int msgLength;
    while (true) {
        // qDebug() << "MidiThreadReceiver polls stream.";
        PmError pollStatus = Pm_Poll(midiInStream);

        if (pollStatus == true) {
            msgLength = Pm_Read(midiInStream, midiEventBuffer, 1);
            if (msgLength > 0) {
                MidiEvent midiData = MidiEvent::fromPortMidiMessage(midiEventBuffer[0]);
                qDebug() << "PortMidi received message:" << midiData.toString();
                emit sigMidiMessageArrived(midiData);
            }
        }
        QThread::msleep(5);
    }
}


















