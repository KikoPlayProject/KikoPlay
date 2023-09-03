#include "lua_appevent.h"
#include "Common/eventbus.h"
#include "Common/logger.h"
#include "Extension/Common/ext_common.h"
#include "Extension/App/kapp.h"

namespace  Extension
{

struct EventListenerRes : public AppRes
{
    EventListenerRes(EventListener *l, int ref) : listener(l), cbRef(ref) {}
    ~EventListenerRes() {  if (listener)  listener->deleteLater(); }
    EventListener *listener;
    int cbRef;
};

void AppEventBusEvent::setup()
{
    const luaL_Reg funcs[] = {
        {"push", push},
        {"listen", listen},
        {"unlisten", unlisten},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "event"}, funcs);
    addDataMembers({"kiko", "event"}, {
       { "EVENT_PLAYER_STATE_CHANGED", EventBus::EVENT_PLAYER_STATE_CHANGED },
       { "EVENT_PLAYER_FILE_CHANGED",  EventBus::EVENT_PLAYER_FILE_CHANGED },
       { "EVENT_LIBRARY_ANIME_ADDED",  EventBus::EVENT_LIBRARY_ANIME_ADDED },
       { "EVENT_LIBRARY_ANIME_UPDATED", EventBus::EVENT_LIBRARY_ANIME_UPDATED },
       { "EVENT_LIBRARY_EP_FINISH", EventBus::EVENT_LIBRARY_EP_FINISH },
       { "EVENT_APP_STYLE_CHANGED", EventBus::EVENT_APP_STYLE_CHANGED },
    });
}

int AppEventBusEvent::push(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) != 2 || lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TTABLE)
    {
        lua_pushstring(L, "event.push: param error");
        return 1;
    }
    const int event = lua_tointeger(L, 1);
    if (event < 100)
    {
        lua_pushstring(L, "event.push: invalid user event");
        return 1;
    }
    const QVariant param = getValue(L, true);
    EventBus::getEventBus()->pushEvent(EventParam{event, param});
    return 0;
}

int AppEventBusEvent::listen(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) != 2 || lua_type(L, 1) != LUA_TNUMBER || lua_type(L, 2) != LUA_TFUNCTION)
    {
        lua_pushstring(L, "event.listen: param error");
        return 1;
    }
    const int event = lua_tointeger(L, 1);
    const int cbRef = luaL_ref(L, LUA_REGISTRYINDEX);
    const QString resKey = QString("event_%1_listener").arg(event);
    AppRes *r = app->getRes(resKey);
    if (r)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, static_cast<EventListenerRes *>(r)->cbRef);
        app->removeRes(resKey);
    }
    r = new EventListenerRes(new EventListener(EventBus::getEventBus(), event, [app, cbRef](const EventParam *p){
        lua_State *L = app->getState();
        if (!L) return;
        lua_rawgeti(L, LUA_REGISTRYINDEX, cbRef);
        if (lua_rawgeti(L, LUA_REGISTRYINDEX, cbRef) == LUA_TFUNCTION)
        {
            pushValue(L, p->param);
            if (lua_pcall(L, 1, 0, 0))
            {
                Logger::logger()->log(Logger::Extension, QString("[%1][event_%2]%3").arg(app->id()).arg(p->eventType).arg(QString(lua_tostring(L, -1))));
            }
        }
        lua_pop(L, 1);
    }), cbRef);
    app->addRes(resKey, r);
    return 0;
}

int AppEventBusEvent::unlisten(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) != 1 || lua_type(L, 1) != LUA_TNUMBER)
    {
        lua_pushstring(L, "event.unlisten: param error");
        return 1;
    }
    const int event = lua_tointeger(L, 1);
    const QString resKey = QString("event_%1_listener").arg(event);
    AppRes *r = app->getRes(resKey);
    if (r)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, static_cast<EventListenerRes *>(r)->cbRef);
        app->removeRes(resKey);
    }
    return 0;
}



}
