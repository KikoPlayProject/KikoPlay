#include "lua_timer.h"
#include "Common/logger.h"

namespace Extension
{

const char *TimerData::MetaName = "meta.kiko.timer";

void Timer::setup()
{
    const luaL_Reg timerMembers[] = {
        {"start",  TimerData::start},
        {"__gc",  TimerData::timerGC},
        {"start",  TimerData::start},
        {"stop",  TimerData::stop},
        {"setinterval",  TimerData::setinterval},
        {"interval",  TimerData::interval},
        {"active",  TimerData::active},
        {"remaining",  TimerData::remaining},
        {"ontimeout",  TimerData::ontimeout},
        {nullptr, nullptr}
    };
    registerMemberFuncs(TimerData::MetaName, timerMembers);

    const luaL_Reg funcs[] = {
        {"create", create},
        {"run", run},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "timer"}, funcs);
}

int Timer::create(lua_State *L)
{
    int interval = -1;
    if (lua_gettop(L) == 1 && lua_isinteger(L, 1))
    {
        interval = lua_tointeger(L, 1);
    }
    TimerData **t = (TimerData **)lua_newuserdata(L, sizeof(TimerData *));
    luaL_getmetatable(L, TimerData::MetaName);
    lua_setmetatable(L, -2);
    TimerData *d =new TimerData(new QTimer);
    *t = d;
    if (interval > 0)
    {
        d->timer->setInterval(interval);
    }
    QObject::connect(d->timer, &QTimer::timeout, [=](){
        if (!d->timeoutRef) return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, d->timeoutRef);
        if (lua_pcall(L, 0, 0, 0))
        {
            Logger::logger()->log(Logger::Extension, "[timer.timeout]" + QString(lua_tostring(L, -1)));
        }
    });
    return 1;
}

int Timer::run(lua_State *L)
{
    if (lua_gettop(L) != 2 || !lua_isinteger(L, 1) || !lua_isfunction(L, 2))
    {
        lua_pushstring(L, "param error: timer.run(timeout, func)");
        return 1;
    }
    const int timeout = lua_tointeger(L, 1);
    const int cb = luaL_ref(L, LUA_REGISTRYINDEX);
    QTimer::singleShot(timeout, [=](){
        lua_rawgeti(L, LUA_REGISTRYINDEX, cb);
        if (lua_pcall(L, 0, 0, 0))
        {
            Logger::logger()->log(Logger::Extension, "[timer.run]" + QString(lua_tostring(L, -1)));
        }
        luaL_unref(L, LUA_REGISTRYINDEX, cb);
    });
    return 0;
}

TimerData *TimerData::checkTimer(lua_State *L, int pos)
{
    void *ud = luaL_checkudata(L, pos, TimerData::MetaName);
    luaL_argcheck(L, ud != NULL, pos, "timer expected");
    return *(TimerData **)ud;
}

int TimerData::timerGC(lua_State *L)
{
    TimerData *t = checkTimer(L, 1);
    if (t)
    {
        if (t->timeoutRef)
        {
            luaL_unref(L, LUA_REGISTRYINDEX, t->timeoutRef);
        }
        delete t;
    }
    return 0;
}

int TimerData::start(lua_State *L)
{
    TimerData *t = checkTimer(L, 1);
    if (!t) return 0;
    t->timer->start();
    return 0;
}

int TimerData::stop(lua_State *L)
{
    TimerData *t = checkTimer(L, 1);
    if (!t) return 0;
    t->timer->stop();
    return 0;
}

int TimerData::setinterval(lua_State *L)
{
    TimerData *t = checkTimer(L, 1);
    if (!t || lua_gettop(L) < 2 || !lua_isinteger(L, 2)) return 0;
    if (lua_gettop(L) == 1 && lua_isinteger(L, 1))
    t->timer->setInterval(lua_tointeger(L, 2));
    return 0;
}

int TimerData::interval(lua_State *L)
{
    TimerData *t = checkTimer(L, 1);
    if (!t) return 0;
    lua_pushinteger(L, t->timer->interval());
    return 1;
}

int TimerData::active(lua_State *L)
{
    TimerData *t = checkTimer(L, 1);
    if (!t) return 0;
    lua_pushboolean(L, t->timer->isActive());
    return 1;
}

int TimerData::remaining(lua_State *L)
{
    TimerData *t = checkTimer(L, 1);
    if (!t) return 0;
    lua_pushinteger(L, t->timer->remainingTime());
    return 1;
}

int TimerData::ontimeout(lua_State *L)
{
    TimerData *t = checkTimer(L, 1);
    if (!t || lua_gettop(L) != 2 || !lua_isfunction(L, 2)) return 0;
    if (t->timeoutRef)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, t->timeoutRef);
    }
    t->timeoutRef = luaL_ref(L, LUA_REGISTRYINDEX);
    return 0;
}



}
