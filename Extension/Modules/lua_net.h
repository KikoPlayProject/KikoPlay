#ifndef LUA_NET_H
#define LUA_NET_H
#include "modulebase.h"
#include "Common/network.h"
namespace Extension
{
class Net : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
public:
    static int httpGet(lua_State *L);
    static int httpHead(lua_State *L);
    static int httpPost(lua_State *L);
    static int httpGetBatch(lua_State *L);
private:
    static void pushNetworkReply(lua_State *L, const Network::Reply &reply);
    static int json2table(lua_State *L);
    static int table2json(lua_State *L);

};
}
#endif // LUA_NET_H
