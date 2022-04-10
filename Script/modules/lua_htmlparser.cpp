#include "lua_htmlparser.h"
#include "Common/htmlparsersax.h"

namespace LuaModule
{

void HtmlParser::setup()
{
    const luaL_Reg htmlParserFuncs[] = {
        {"htmlparser", htmlparser},
        {nullptr, nullptr}
    };
    registerFuncs("kiko", htmlParserFuncs);

    const luaL_Reg memberFuncs [] = {
        {"adddata", hpAddData},
        {"seekto", hpSeekTo},
        {"atend", hpEnd},
        {"readnext", hpReadNext},
        {"curpos", hpCurPos},
        {"readcontent", hpReadContentText},
        {"readuntil", hpReadContentUntil},
        {"start", hpIsStartNode},
        {"curnode", hpCurrentNode},
        {"curproperty", hpCurrentNodeProperty},
        {"__gc", htmlParserGC},
        {nullptr, nullptr}
    };
    registerMemberFuncs("meta.kiko.htmlparser", memberFuncs);
}

int HtmlParser::htmlparser(lua_State *L)
{
    int n = lua_gettop(L);
    const char *data = nullptr;
    size_t length = 0;
    if(n > 0 && lua_type(L, 1)==LUA_TSTRING)
    {
        data = lua_tolstring(L, -1, &length);
    }
    HTMLParserSax **parser = (HTMLParserSax **)lua_newuserdata(L, sizeof(HTMLParserSax *));
    luaL_getmetatable(L, "meta.kiko.htmlparser");
    lua_setmetatable(L, -2);  // reader meta
    *parser = new HTMLParserSax(QByteArray(data, length)); //reader
    return 1;
}

HTMLParserSax *HtmlParser::checkHTMLParser(lua_State *L)
{
    void *ud = luaL_checkudata(L, 1, "meta.kiko.htmlparser");
    luaL_argcheck(L, ud != NULL, 1, "`kiko.htmlparser' expected");
    return *(HTMLParserSax **)ud;
}

int HtmlParser::hpAddData(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    if(lua_gettop(L) > 1 && lua_type(L, 2)==LUA_TSTRING)
    {
        size_t len = 0;
        const char *data = lua_tolstring(L, 2, &len);
        parser->addData(QByteArray(data, len));
    }
    return 0;
}

int HtmlParser::hpSeekTo(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    int n = lua_gettop(L);
    if(n!=2 || lua_type(L, 2)!=LUA_TNUMBER)
    {
        return 0;
    }
    int pos = lua_tonumber(L, 2);
    parser->seekTo(pos);
    return 0;
}

int HtmlParser::hpReadNext(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    parser->readNext();
    return 0;
}

int HtmlParser::hpEnd(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    bool atEnd = parser->atEnd();
    lua_pushboolean(L, atEnd);
    return 1;
}

int HtmlParser::hpCurPos(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    int pos = parser->curPos();
    lua_pushnumber(L, pos);
    return 1;
}

int HtmlParser::hpReadContentText(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    QByteArray content(parser->readContentText());
    lua_pushlstring(L, content.constData(), content.size());
    return 1;
}

int HtmlParser::hpReadContentUntil(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    int n = lua_gettop(L);
    if(n!=3 || lua_type(L, 2)!=LUA_TSTRING || lua_type(L, 3)!=LUA_TBOOLEAN)
    {
        lua_pushnil(L);
        return 1;
    }
    QByteArray nodeName(lua_tostring(L, 2));
    bool isStart = lua_toboolean(L, 3);
    QByteArray content(parser->readContentUntil(nodeName, isStart));
    lua_pushlstring(L, content.constData(), content.size());
    return 1;
}

int HtmlParser::hpIsStartNode(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    lua_pushboolean(L, parser->isStartNode());
    return 1;
}

int HtmlParser::hpCurrentNode(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    lua_pushstring(L, parser->currentNode().constData());
    return 1;
}

int HtmlParser::hpCurrentNodeProperty(lua_State *L)
{
    HTMLParserSax *parser = checkHTMLParser(L);
    int n = lua_gettop(L);
    if(n!= 2 || lua_type(L, 2)!=LUA_TSTRING)
    {
        lua_pushnil(L);
        return 1;
    }
    QByteArray nodeName(lua_tostring(L, 2));
    lua_pushstring(L, parser->currentNodeProperty(nodeName).constData());
    return 1;
}

int HtmlParser::htmlParserGC(lua_State *L) {
    HTMLParserSax *parser = checkHTMLParser(L);
    if(parser) delete parser;
    return 0;
}

}
