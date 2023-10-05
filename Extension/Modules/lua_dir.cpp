#include "lua_dir.h"
#include <QDir>
#include <QFileInfo>
#include <QDateTime>
#include "Extension/Common/ext_common.h"

namespace  Extension
{

void Dir::setup()
{
    const luaL_Reg funcs[] = {
        {"fileinfo", fileinfo},
        {"exists", exists},
        {"mkpath", mkpath},
        {"rmpath", rmpath},
        {"entrylist", entrylist},
        {"rename", rename},
        {"syspath", syspath},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "dir"}, funcs);
    addDataMembers({"kiko", "dir"}, {
        { "FILTER_DIRS", QDir::Dirs },
        { "FILTER_ALL_DIRS", QDir::AllDirs },
        { "FILTER_FILES", QDir::AllDirs },
        { "FILTER_DRIVES", QDir::Drives },
        { "FILTER_NO_SYMLINKS", QDir::NoSymLinks },
        { "FILTER_NO_DOT", QDir::NoDotAndDotDot },
        { "FILTER_ALL_ENTRIES", QDir::AllEntries},
        { "FILTER_HIDDEN", QDir::Hidden},
        { "SORT_NAME", QDir::Name},
        { "SORT_TIME", QDir::Time},
        { "SORT_SIZE", QDir::Size},
        { "SORT_TYPE", QDir::Type},
        { "SORT_NO", QDir::NoSort},
        { "SORT_DIR_FIRST", QDir::DirsFirst},
        { "SORT_DIR_FIRST", QDir::DirsFirst},
        { "SORT_DIR_LAST", QDir::DirsLast},
        { "SORT_REVERSE", QDir::Reversed},
        { "SORT_IGNORE_CASE", QDir::IgnoreCase},
    });
}

int Dir::fileinfo(lua_State *L)
{
    if (lua_gettop(L) != 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    const QString path = lua_tostring(L, 1);
    QFileInfo info{path};
    const QVariantMap infoMap = {
        {"absolutePath", info.absolutePath()},
        {"absoluteFilePath", info.absoluteFilePath()},
        {"baseName", info.baseName()},
        {"fileName", info.fileName()},
        {"completeSuffix", info.completeSuffix()},
        {"suffix", info.suffix()},
        {"exists", info.exists()},
        {"isDir", info.isDir()},
        {"isFile", info.isFile()},
        {"size", info.size()},
        {"birthTime", info.birthTime().toMSecsSinceEpoch()},
        {"lastModified", info.lastModified().toMSecsSinceEpoch()},
        {"lastRead", info.lastRead().toMSecsSinceEpoch()},
    };
    pushValue(L, infoMap);
    return 1;
}

int Dir::exists(lua_State *L)
{
    if (lua_gettop(L) != 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    lua_pushboolean(L, QFileInfo::exists(lua_tostring(L, 1)));
    return 1;
}

int Dir::mkpath(lua_State *L)
{
    if (lua_gettop(L) != 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    QDir dir;
    lua_pushboolean(L, dir.mkpath(lua_tostring(L, 1)));
    return 1;
}

int Dir::rmpath(lua_State *L)
{
    if (lua_gettop(L) != 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    QDir dir;
    lua_pushboolean(L, dir.rmpath(lua_tostring(L, 1)));
    return 1;
}

int Dir::entrylist(lua_State *L)
{
    const int n = lua_gettop(L);
    if (n < 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    // entrylist(path, namefilter, filter, sort)
    const QString path = lua_tostring(L, 1);
    QDir dir(path);
    if (n > 1 && lua_type(L, 2) != LUA_TSTRING)
    {
        dir.setNameFilters(QString(lua_tostring(L, 2)).split(";"));
    }
    if (n > 2 && lua_isinteger(L, 3))
    {
        QDir::Filters filter((int)lua_tointeger(L, 3));
        dir.setFilter(filter);
    }
    if (n > 3 && lua_isinteger(L, 4))
    {
        QDir::SortFlags sort((int)lua_tointeger(L, 4));
        dir.setSorting(sort);
    }
    pushValue(L, dir.entryList());
    return 1;
}

int Dir::rename(lua_State *L)
{
    if (lua_gettop(L) != 2 || lua_type(L, 1) != LUA_TSTRING || lua_type(L, 2) != LUA_TSTRING)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    QDir dir;
    lua_pushboolean(L, dir.rename(lua_tostring(L, 1), lua_tostring(L, 2)));
    return 1;
}

int Dir::syspath(lua_State *L)
{
    QStringList drives;
    for (const auto &info : QDir::drives())
    {
        drives.append(info.absolutePath());
    }
    const QVariantMap pathinfo = {
        {"kiko", QDir::currentPath()},
        {"home", QDir::homePath()},
        {"root", QDir::rootPath()},
        {"temp", QDir::tempPath()},
        {"drives", drives},
    };
    pushValue(L, pathinfo);
    return 1;
}

}
