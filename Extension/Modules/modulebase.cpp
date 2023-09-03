#include "modulebase.h"
#include "Extension/Common/ext_common.h"
#include "Common/logger.h"
namespace Extension
{

ModuleBase::ModuleBase(lua_State *state) : L(state)
{

}

void ModuleBase::registerFuncs(const char *tname, const luaL_Reg *funcs)
{
    if(!L) return;
    lua_getglobal(L, tname);
    if (lua_isnil(L, -1)) {
      lua_pop(L, 1);
      lua_newtable(L);
    }
    luaL_setfuncs(L, funcs, 0);
    lua_setglobal(L, tname);
}

void ModuleBase::registerFuncs(const QByteArrayList &tnames, const luaL_Reg *funcs)
{
    if (!L || tnames.empty()) return;
    lua_getglobal(L, tnames[0].constData());
    if (lua_isnil(L, -1))
    {
      lua_pop(L, 1);
      lua_newtable(L);  //  t
      lua_pushvalue(L, -1);   // t  t
      lua_setglobal(L, tnames[0].constData());  // t
    }
    for (int i = 1; i < tnames.size(); ++i)
    {
        if (lua_getfield(L, -1, tnames[i].constData()) == LUA_TNIL)
        {
            lua_pop(L, 1);
            lua_newtable(L);  //  t  t_i
            lua_pushvalue(L, -1);   // t  t_i  t_i
            lua_setfield(L, 1, tnames[i].constData());   // t  t_i
        }
        lua_remove(L, 1);  // t_i
    }
    luaL_setfuncs(L, funcs, 0);
    lua_pop(L, 1);
}

void ModuleBase::addDataMembers(const QByteArrayList &tnames, const QVector<QPair<QString, QVariant> > members)
{
    if (!L || tnames.empty()) return;
    lua_getglobal(L, tnames[0].constData());
    if (lua_isnil(L, -1))
    {
      lua_pop(L, 1);
      lua_newtable(L);  //  t
      lua_pushvalue(L, -1);   // t  t
      lua_setglobal(L, tnames[0].constData());  // t
    }
    for (int i = 1; i < tnames.size(); ++i)
    {
        if (lua_getfield(L, -1, tnames[i].constData()) == LUA_TNIL)
        {
            lua_pop(L, 1);
            lua_newtable(L);  //  t  t_i
            lua_pushvalue(L, -1);   // t  t_i  t_i
            lua_setfield(L, 1, tnames[i].constData());   // t  t_i
        }
        lua_remove(L, 1);  // t_i
    }
    for (const auto &p : members)
    {
        lua_pushstring(L, p.first.toStdString().c_str()); // table key
        pushValue(L, p.second); // table key value
        lua_rawset(L, -3);
    }
    lua_pop(L, 1);
}

void ModuleBase::registerMemberFuncs(const char *metaName, const luaL_Reg *funcs)
{
    if(!L) return;
    luaL_newmetatable(L, metaName);
    lua_pushstring(L, "__index");
    lua_pushvalue(L, -2); // pushes the metatable
    lua_rawset(L, -3); // metatable.__index = metatable
    luaL_setfuncs(L, funcs, 0);
    lua_pop(L, 1);
}

void ModuleBase::logInfo(const QString &info, const QString &scriptId)
{
    Logger::logger()->log(Logger::Script, QString("[%1]%2").arg(scriptId, info));
}

void ModuleBase::logError(const QString &info, const QString &scriptId)
{
    Logger::logger()->log(Logger::Script, QString("[ERROR][%1]%2").arg(scriptId, info));
}

}
