#include "EXMIDIListener.h"
#include <QMetaObject>
#include <QDebug>

MidiListener::MidiListener(QObject* parent)
    : QObject(parent), midiIn(new RtMidiIn())
{
    midiIn->ignoreTypes(false, false, false);
}

MidiListener::~MidiListener()
{
    closePort();
    delete midiIn;
}

QStringList MidiListener::availableInputPorts() const
{
    QStringList ports;
    unsigned int nPorts = midiIn->getPortCount();
    for (unsigned int i = 0; i < nPorts; ++i) {
        ports << QString::fromStdString(midiIn->getPortName(i));
    }
    return ports;
}

void MidiListener::openPort(int index)
{
    closePort();  // Close any existing port
    try {
        midiIn->openPort(index);
        midiIn->setCallback(&MidiListener::midiCallback, this);
    } catch (RtMidiError& e) {
        emit sigErrorOccurred(QString::fromStdString(e.getMessage()));
    }
}

void MidiListener::closePort()
{
    if (midiIn->isPortOpen()) {
        midiIn->closePort();
    }
}

void MidiListener::midiCallback(double, std::vector<unsigned char>* message, void* userData)
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



















