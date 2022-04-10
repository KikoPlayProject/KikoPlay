#ifndef LUA_NET_H
#define LUA_NET_H
#include "modulebase.h"
#include "Common/network.h"
namespace LuaModule
{
class Net : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static void pushNetworkReply(lua_State *L, const Network::Reply &reply);
    static int httpGet(lua_State *L);
    static int httpPost(lua_State *L);
    static int httpGetBatch(lua_State *L);
    static int json2table(lua_State *L);
    static int table2json(lua_State *L);

};
}
#endif // LUA_NET_H
