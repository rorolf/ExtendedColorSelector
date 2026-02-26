

#pragma once
#include <QObject>
#include <QHash>
#include <QPointer>
#include <QVariant>


// Since there will be a variable number of signals to be reacted to,
// each one needs their own Object that emits on this one signal only
// With Actionchannels owned by ActionBus, any signal can be split into multiple signals
class ActionChannel : public QObject {
        Q_OBJECT
    public:
        using QObject::QObject;
    Q_SIGNALS:
        void triggered(const QVariant& value);
    };

    class ActionBus : public QObject {
        Q_OBJECT
    public:
        explicit ActionBus(QObject* parent = nullptr) : QObject(parent) {}

        ActionChannel* channel(const QString& name) {
            if (auto ch = m_channels.value(name)) return ch;
            auto* ch = new ActionChannel(this);
            ch->setObjectName(name);
            m_channels.insert(name, ch);
            return ch;
        }

        template<typename T>
        void emitValue(const QString& name, const T& v) {
            emit channel(name)->triggered(QVariant::fromValue(v));
        }

        void emitVariant(const QString& name, const QVariant& v) {
            emit channel(name)->triggered(v);
        }

    private:
        QHash<QString, QPointer<ActionChannel>> m_channels;
};




