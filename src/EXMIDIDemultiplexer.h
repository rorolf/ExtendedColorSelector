

#pragma once

#include <QObject>
#include <QHash>
#include <QPointer>
#include <QVariant>
#include "EXMIDIEvent.h"


// Since there will be a variable number of signals to be reacted to,
// each one needs their own Object that emits on this one signal only
// With Actionchannels owned by Demultiplexer, any signal can be split into multiple signals
class MIDIChannel : public QObject
{
        Q_OBJECT
    public:
        using QObject::QObject;
    Q_SIGNALS:
        void triggered(const int& value);
};

class EXMIDIDemultiplexer : public QObject
{
        Q_OBJECT
    public:
        explicit EXMIDIDemultiplexer(QObject* parent = nullptr) : QObject(parent) {}

        MIDIChannel* channel(const QString& name) {
            if (auto ch = m_channels.value(name)) return ch;
            auto* ch = new MIDIChannel(this);
            ch->setObjectName(name);
            m_channels.insert(name, ch);
            return ch;
        }

        void emitValue(const QString& name, const int& v) {
            emit channel(name)->triggered(v);
        }

        // void emitVariant(const QString& name, const QVariant& v) {
        //     emit channel(name)->triggered(v);
        // }

    private:
        QHash<QString, QPointer<MIDIChannel>> m_channels;
};




