#include "lua_storageinterface.h"
#include "Extension/App/appstorage.h"
#include "Extension/App/kapp.h"

namespace Extension
{
const char *StorageInterface::resKey = "storage";
void StorageInterface::setup()
{
    const luaL_Reg funcs[] = {
        {"set", set},
        {"get", get},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "storage"}, funcs);
}

int StorageInterface::set(lua_State *L)
{
    if (lua_gettop(L) < 2 || lua_type(L, 1) != LUA_TSTRING) return 0;
    AppStorage *s = getStorage(L);
    if (!s) return 0;
    const QString key = lua_tostring(L, 1);
    const QVariant val = getValue(L, false);
    s->set(key, val);
    return 0;
}

int StorageInterface::get(lua_State *L)
{
    if (lua_gettop(L) < 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    AppStorage *s = getStorage(L);
    if (!s)
    {
        lua_pushnil(L);
        return 1;
    }
    pushValue(L, s->get(lua_tostring(L, 1)));
    return 1;
}

AppStorage *StorageInterface::getStorage(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    AppRes *res = app->getRes(resKey);
    AppStorage *s = nullptr;
    if (!res)
    {
        s = new AppStorage(app->dataPath() + "/" + resKey);
        app->addRes(resKey, new StorageRes(s));
    }
    else
    {
        s = static_cast<StorageRes *>(res)->storage;
    }
    return s;
}



}
