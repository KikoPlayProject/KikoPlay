#include "lua_stringutil.h"
#include <cstring>

namespace  Extension
{

void StringUtil::setup()
{
    const luaL_Reg strFuncs[] = {
        {"trim", trim},
        {"startswith", startswith},
        {"endswith", endswith},
        {"split", split},
        {"indexof", indexof},
        {"lastindexof", lastindexof},
        {"encode", encode},
        {nullptr, nullptr}
    };
    registerFuncs("string", strFuncs);
    addDataMembers({"string"}, {
        { "CODE_LOCAL", CodeType::LOCAL },
        { "CODE_UTF8", CodeType::UTF_8 },
    });
}

int StringUtil::trim(lua_State *L)
{
    const char* front;
    const char* end;
    size_t size;
    front = luaL_checklstring(L, 1, &size);
    end = &front[size - 1];

    while (size && isspace((unsigned char)*front))
    {
      size--;
      front++;
    }

    while (size && isspace((unsigned char)*end))
    {
      size--;
      end--;
    }
    lua_pushlstring(L,front,(size_t)(end - front) + 1);
    return 1;
}

int StringUtil::startswith(lua_State *L)
{
    const char *string = luaL_checkstring(L, 1);
    int string_len = lua_rawlen(L, 1);

    const char *token = luaL_checkstring(L, 2);
    int token_len = lua_rawlen(L, 2);
    int start = 0;
    if (token_len <= string_len)
        start = memcmp(string, token, token_len) == 0;
    lua_pushboolean(L, start);
    return 1;
}

int StringUtil::endswith(lua_State *L)
{
    size_t string_len;
    const char *string = luaL_checklstring(L, 1, &string_len);

    size_t token_len;
    const char *token = luaL_checklstring(L, 2, &token_len);

    int end = 0;
    if (token_len <= string_len)
    {
        string += string_len - token_len;
        end = memcmp(string, token, token_len) == 0;
    }
    lua_pushboolean(L, end);
    return 1;
}

int StringUtil::split(lua_State *L)
{
    const char *s = luaL_checkstring(L, 1);
    size_t len = 0;
    const char *sep = luaL_checklstring(L, 2, &len);
    const char *e = NULL;
    bool skipEmpty = false;
    if (lua_gettop(L) == 3)
    {
        skipEmpty = lua_toboolean(L, 3);
    }
    int i = 1;

    lua_newtable(L);  /* result */

    /* repeat for each separator */
    while ((e = strstr(s, sep)) != NULL)
    {
        if (!skipEmpty || e-s > 0)
        {
            lua_pushlstring(L, s, e-s);  /* push substring */
            lua_rawseti(L, -2, i++);
        }
        s = e + len;  /* skip separator */
    }

    /* push last substring */
    lua_pushstring(L, s);
    lua_rawseti(L, -2, i);

    return 1;  /* return the table */
}

int StringUtil::indexof(lua_State *L)
{
    size_t size = 0;
    const char* str = luaL_checklstring(L, 1, &size);;
    size_t target_size = 0;
    const char *target = luaL_checklstring(L, 2, &target_size);
    int from = 0;
    if (lua_gettop(L) > 2)
    {
        from = lua_tointeger(L, 3) - 1;
    }
    const QByteArray srcStr(str, size);
    int index = srcStr.indexOf(QByteArray(target, target_size), from);
    if (index >= 0) index += 1;
    lua_pushinteger(L, index);
    return 1;
}

int StringUtil::lastindexof(lua_State *L)
{
    size_t size = 0;
    const char* str = luaL_checklstring(L, 1, &size);;
    size_t target_size = 0;
    const char *target = luaL_checklstring(L, 2, &target_size);
    int from = -1;
    if (lua_gettop(L) > 2)
    {
        from = lua_tointeger(L, 3) - 1;
    }
    const QByteArray srcStr(str, size);
    int index = srcStr.lastIndexOf(QByteArray(target, target_size), from);
    if (index >= 0) index += 1;
    lua_pushinteger(L, index);
    return 1;
}

int StringUtil::encode(lua_State *L)
{
    size_t size = 0;
    const char* str = luaL_checklstring(L, 1, &size);;
    int src_code = CodeType::UTF_8;
    int dest_code = CodeType::UTF_8;
    if (lua_gettop(L) > 1)
    {
        src_code = lua_tointeger(L, 2);
    }
    if (lua_gettop(L) > 2)
    {
        dest_code = lua_tointeger(L, 3);
    }
    QString src;
    if (src_code == CodeType::LOCAL)
    {
        src = QString::fromLocal8Bit(str, size);
    }
    else
    {
        src = QString::fromUtf8(str, size);
    }
    if (dest_code == CodeType::LOCAL)
    {
        QByteArray d = src.toLocal8Bit();
        lua_pushlstring(L, d.constData(), d.size());
    }
    else
    {
        QByteArray d = src.toUtf8();
        lua_pushlstring(L, d.constData(), d.size());
    }
    return 1;
}

}
