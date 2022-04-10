#ifndef LUA_XMLREADER_H
#define LUA_XMLREADER_H
#include "modulebase.h"
class QXmlStreamReader;
namespace LuaModule
{
class XmlReader : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int xmlreader (lua_State *L);
    static QXmlStreamReader *checkXmlReader(lua_State *L);
    static int xrAddData(lua_State *L);
    static int xrClear(lua_State *L);
    static int xrEnd(lua_State *L);
    static int xrReadNext(lua_State *L);
    static int xrStartElem(lua_State *L);
    static int xrEndElem(lua_State *L);
    static int xrName(lua_State *L);
    static int xrAttr(lua_State *L);
    static int xrHasAttr(lua_State *L);
    static int xrReadElemText(lua_State *L);
    static int xrError(lua_State *L);
    static int xmlreaderGC (lua_State *L);
};
}
#endif // LUA_XMLREADER_H
