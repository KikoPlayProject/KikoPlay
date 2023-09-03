#ifndef TIMER_H
#define TIMER_H

#include "modulebase.h"
#include <QTimer>

namespace Extension
{
struct TimerData
{
    TimerData(QTimer *t) : timer(t) {}
    ~TimerData()
    {
        timer->deleteLater();
        timer = nullptr;
    }
    TimerData &operator=(const TimerData&) = delete;
    TimerData(const TimerData&) = delete;

    QTimer *timer = nullptr;
    int timeoutRef = 0;

    static const char *MetaName;
    static TimerData *checkTimer(lua_State *L, int pos);

    static int timerGC(lua_State *L);
    static int start(lua_State *L);
    static int stop(lua_State *L);
    static int setinterval(lua_State *L);
    static int interval(lua_State *L);
    static int active(lua_State *L);
    static int remaining(lua_State *L);

    static int ontimeout(lua_State *L);
};

class Timer : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int create(lua_State *L);
    static int run(lua_State *L);
};
}
#endif // TIMER_H
