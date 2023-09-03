#ifndef EVENTBUS_H
#define EVENTBUS_H

#include <QObject>
#include <QEvent>
#include <QVariant>
#include <QSharedPointer>
#include <QReadWriteLock>

#define EventBusEventId QEvent::User + 1
class EventBus;
struct EventParam
{
    int eventType;
    QVariant param;
};

class EventBusEvent : public QEvent
{
public:
    EventBusEvent(QSharedPointer<EventParam> param) : QEvent(QEvent::Type(EventBusEventId)), eventParam(param) {}
    QSharedPointer<EventParam> eventParam;
};

class EventListener : public QObject
{
    Q_OBJECT
public:
    using EventCallback = std::function<void(const EventParam *)>;
    explicit EventListener(EventBus *bus, int event, const EventCallback &cb,  QObject *parent = nullptr);
    ~EventListener();
    int eventType() const { return e; }
    bool event(QEvent *event);
private:
    EventCallback eCb;
    int e;
    EventBus *ebus;
};

class EventBus : public QObject
{
    Q_OBJECT
public:
    enum EventType
    {
        EVENT_PLAYER_STATE_CHANGED = 1,
        EVENT_PLAYER_FILE_CHANGED,
        EVENT_LIBRARY_ANIME_ADDED,
        EVENT_LIBRARY_ANIME_UPDATED,
        EVENT_LIBRARY_EP_FINISH,
        EVENT_APP_STYLE_CHANGED,
        EVENT_UNKNOWN
    };
public:
    static EventBus *getEventBus();
    void pushEvent(const EventParam &param);
    void addListener(EventListener *listener);
    void removeListener(EventListener *listener);
    bool hasListener(int event) const { return eventListener.contains(event) && !eventListener[event].empty(); }
private:
    explicit EventBus(QObject *parent = nullptr);
    QHash<int, QVector<EventListener *>> eventListener;
    QReadWriteLock lock;
};

#endif // EVENTBUS_H
