#ifndef DANMUINTERFACE_H
#define DANMUINTERFACE_H

#include "modulebase.h"
struct DanmuComment;
class Pool;
namespace Extension
{

class DanmuInterface : public ModuleBase
{
public:
    using ModuleBase::ModuleBase;
    virtual void setup();
private:
    static int launch(lua_State *L);
    static int getpool(lua_State *L);
    static int addpool(lua_State *L);
    static int updatepool(lua_State *L);
    static int getdanmu(lua_State *L);
    static int addsrc(lua_State *L);

    static Pool *getPoolFromParam(lua_State *L);
    static DanmuComment *parseDanmu(const QVariantMap &dobj);
};
}
#endif // DANMUINTERFACE_H
