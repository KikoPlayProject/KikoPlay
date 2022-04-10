#include "lua_xmlreader.h"
#include <QXmlStreamReader>

namespace LuaModule
{

void XmlReader::setup()
{
    const luaL_Reg xmlReaderFunc[] = {
        {"xmlreader", xmlreader},
        {nullptr, nullptr}
    };
    registerFuncs("kiko", xmlReaderFunc);

    const luaL_Reg memberFuncs[] = {
        {"adddata", xrAddData},
        {"clear", xrClear},
        {"atend", xrEnd},
        {"readnext", xrReadNext},
        {"startelem", xrStartElem},
        {"endelem", xrEndElem},
        {"name", xrName},
        {"attr", xrAttr},
        {"hasattr", xrHasAttr},
        {"elemtext", xrReadElemText},
        {"error", xrError},
        {"__gc", xmlreaderGC},
        {nullptr, nullptr}
    };
    registerMemberFuncs("meta.kiko.xmlreader", memberFuncs);
}

int XmlReader::xmlreader(lua_State *L)
{
    int n = lua_gettop(L);
    const char *data = nullptr;
    if(n > 0 && lua_type(L, 1)==LUA_TSTRING)
    {
        data = lua_tostring(L, 1);
    }
    QXmlStreamReader **reader = (QXmlStreamReader **)lua_newuserdata(L, sizeof(QXmlStreamReader *));
    luaL_getmetatable(L, "meta.kiko.xmlreader");
    lua_setmetatable(L, -2);  // reader meta
    *reader = new QXmlStreamReader(data); //reader
    return 1;
}

QXmlStreamReader *XmlReader::checkXmlReader(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "meta.kiko.xmlreader");
    luaL_argcheck(L, ud != NULL, 1, "`kiko.xmlreader' expected");
    return *(QXmlStreamReader **)ud;
}

int XmlReader::xrAddData(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    if(lua_gettop(L) > 1 && lua_type(L, 2)==LUA_TSTRING)
    {
        size_t len = 0;
        const char *data = lua_tolstring(L, 2, &len);
        reader->addData(QByteArray(data, len));
    }
    return 0;
}

int XmlReader::xrClear(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    reader->clear();
    return 0;
}

int XmlReader::xrEnd(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool atEnd = reader->atEnd();
    lua_pushboolean(L, atEnd);
    return 1;
}

int XmlReader::xrReadNext(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    reader->readNext();
    return 0;
}

int XmlReader::xrStartElem(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool isStartElem = reader->isStartElement();
    lua_pushboolean(L, isStartElem);
    return 1;
}

int XmlReader::xrEndElem(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    bool isEndElem = reader->isEndElement();
    lua_pushboolean(L, isEndElem);
    return 1;
}

int XmlReader::xrName(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    lua_pushstring(L, reader->name().toString().toStdString().c_str());
    return 1;
}

int XmlReader::xrAttr(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    if(lua_gettop(L) == 1 || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    const char *attrName = lua_tostring(L, 2);
    lua_pushstring(L, reader->attributes().value(attrName).toString().toStdString().c_str());
    return 1;
}

int XmlReader::xrHasAttr(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    if(lua_gettop(L) == 1 || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    const char *attrName = lua_tostring(L, 2);
    lua_pushboolean(L, reader->attributes().hasAttribute(attrName));
    return 1;
}

int XmlReader::xrReadElemText(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    lua_pushstring(L, reader->readElementText().toStdString().c_str());
    return 1;
}

int XmlReader::xrError(lua_State *L)
{
    QXmlStreamReader *reader = checkXmlReader(L);
    QString errInfo = reader->errorString();
    if(errInfo.isEmpty()) lua_pushnil(L);
    else lua_pushstring(L, errInfo.toStdString().c_str());
    return 1;
}

int XmlReader::xmlreaderGC(lua_State *L) {
    QXmlStreamReader *reader = checkXmlReader(L);
    if(reader) delete reader;
    return 0;
}

}
