#ifndef LUA_HTMLPARSER_H
#define LUA_HTMLPARSER_H

#include "modulebase.h"
class HTMLParserSax;
namespace Extension
{
class HtmlParser : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int htmlparser(lua_State *L);
    static HTMLParserSax *checkHTMLParser(lua_State *L);
    static int hpAddData(lua_State *L);
    static int hpSeekTo(lua_State *L);
    static int hpReadNext(lua_State *L);
    static int hpEnd(lua_State *L);
    static int hpCurPos(lua_State *L);
    static int hpReadContentText(lua_State *L);
    static int hpReadContentUntil(lua_State *L);
    static int hpIsStartNode(lua_State *L);
    static int hpCurrentNode(lua_State *L);
    static int hpCurrentNodeProperty(lua_State *L);
    static int htmlParserGC (lua_State *L);
};
}
#endif // LUA_HTMLPARSER_H
