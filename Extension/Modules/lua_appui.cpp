#include "lua_appui.h"
#include "Extension/App/kapp.h"

namespace  Extension
{

void AppUI::setup()
{
    const luaL_Reg funcs[] = {
        {"get", get},
        {"remove", remove},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "ui"}, funcs);
}

int AppUI::get(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) < 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    AppWidget *w = app->getWidget(lua_tostring(L, 1));
    if (!w)
    {
        lua_pushnil(L);
        return 1;
    }
    w->luaPush(L);
    return 1;
}

int AppUI::remove(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    if (!app || lua_gettop(L) < 1) return 0;
    AppWidget *w = nullptr;
    if (lua_type(L, 1) == LUA_TSTRING)
    {
       w = app->getWidget(lua_tostring(L, 1));
    }
    else
    {
        w = AppWidget::checkWidget(L, 1, AppWidget::MetaName);
        if (w)
        {
            lua_pushnil(L);
            lua_setmetatable(L, 1);  // widget meta
        }
    }
    if (w)
    {
        w->deleteLater();
    }
    return 0;
}


}
