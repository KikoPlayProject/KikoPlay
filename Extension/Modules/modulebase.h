#ifndef MODULEBASE_H
#define MODULEBASE_H
#include "Extension/Lua/lua.hpp"
#include <QString>
#include <QByteArrayList>
#include <QVariant>
class ScriptBase;
namespace Extension
{
class ModuleBase
{
public:
    ModuleBase(lua_State *state);
    virtual void setup() = 0;

protected:
    lua_State *L;
    void registerFuncs(const char *tname, const luaL_Reg *funcs);
    void registerFuncs(const QByteArrayList &tnames, const luaL_Reg *funcs);
    void addDataMembers(const QByteArrayList &tnames, const QVector<QPair<QString, QVariant>> members);
    void registerMemberFuncs(const char *metaName, const luaL_Reg *funcs);
    static void logInfo(const QString &info, const QString &scriptId);
    static void logError(const QString &info, const QString &scriptId);
};
}
#endif // MODULEBASE_H
