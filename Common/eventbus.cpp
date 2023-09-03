#include "eventbus.h"
#include <QCoreApplication>

EventBus::EventBus(QObject *parent)
    : QObject{parent}
{
}

EventBus *EventBus::getEventBus()
{
    static EventBus eb;
    return &eb;
}

void EventBus::pushEvent(const EventParam &param)
{
    lock.lockForRead();
    const QVector<EventListener *> listeners = eventListener[param.eventType];
    if (!listeners.empty())
    {
        QSharedPointer<EventParam> p = QSharedPointer<EventParam>::create(param);
        for (EventListener *l : listeners)
        {
            QCoreApplication::postEvent(l, new EventBusEvent(p));
        }
    }
    lock.unlock();
}

void EventBus::addListener(EventListener *listener)
{
    lock.lockForWrite();
    QVector<EventListener *> &listeners = eventListener[listener->eventType()];
    if (!listeners.contains(listener))
    {
        listeners.append(listener);
    }
    lock.unlock();
}

void EventBus::removeListener(EventListener *listener)
{
    lock.lockForWrite();
    QVector<EventListener *> &listeners = eventListener[listener->eventType()];
    listeners.removeOne(listener);
    lock.unlock();
}

EventListener::EventListener(EventBus *bus, int event, const EventCallback &cb, QObject *parent) :
    QObject(parent), eCb(cb), e(event), ebus(bus)
{
    ebus->addListener(this);
}

EventListener::~EventListener()
{
    ebus->removeListener(this);
}

bool EventListener::event(QEvent *event)
{
    if (event->type() == EventBusEventId)
    {
        EventBusEvent *e = static_cast<EventBusEvent *>(event);
        if (e->eventParam->eventType == this->e)
        {
            eCb(e->eventParam.get());
            return true;
        }
    }
    return QObject::event(event);
}
