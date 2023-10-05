#include "lua_playerinterface.h"
#include "Extension/Common/ext_common.h"
#include "Play/playcontext.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "Common/notifier.h"

namespace Extension
{

void PlayerInterface::setup()
{
    const luaL_Reg funcs[] = {
        {"setmedia", setmedia},
        {"curfile", curfile},
        {"property", property},
        {"command", command},
        {"optgroups", optgroups},
        {"setoptgroup", setoptgroup},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "player"}, funcs);
}

int PlayerInterface::setmedia(lua_State *L)
{
    if (lua_gettop(L) < 1)
    {
        return 0;
    }
    const int type = lua_type(L, 1);
    if (type == LUA_TSTRING)
    {
        const QString path = lua_tostring(L, 1);
        QMetaObject::invokeMethod(PlayContext::context(), [=](){
            PlayContext::context()->playURL(path);
        }, Qt::BlockingQueuedConnection);
    }
    return 0;
}

int PlayerInterface::curfile(lua_State *L)
{
    const QString file = PlayContext::context()->path;
    if (file.isEmpty()) lua_pushnil(L);
    else lua_pushstring(L, file.toStdString().c_str());
    return 1;
}

int PlayerInterface::property(lua_State *L)
{
    if (lua_gettop(L) < 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        return 0;
    }
    const QString property = QString(lua_tostring(L, 1)).trimmed();
    if(property.isEmpty())
    {
        return 0;
    }
    int errCode = 0;
    const QVariant val = GlobalObjects::mpvplayer->getMPVPropertyVariant(property, errCode);
    lua_pushnumber(L, errCode);
    pushValue(L, val);
    return 2;
}

int PlayerInterface::command(lua_State *L)
{
    if (lua_gettop(L) < 1)
    {
        return 0;
    }
    const QVariant param = getValue(L, true, 10);
    int ret = GlobalObjects::mpvplayer->runCommand(param);
    lua_pushinteger(L, ret);
    return 1;
}

int PlayerInterface::optgroups(lua_State *L)
{
    const QVariantMap optionGroupInfo = {
        {"all", GlobalObjects::mpvplayer->allOptionGroups()},
        {"current", GlobalObjects::mpvplayer->currentOptionGroup()},
    };
    pushValue(L, optionGroupInfo);
    return 1;
}

int PlayerInterface::setoptgroup(lua_State *L)
{
    if (lua_gettop(L) < 1)
    {
        return 0;
    }
    const QString group = lua_tostring(L, 1);
    QMetaObject::invokeMethod(GlobalObjects::mpvplayer, [=](){
        if(GlobalObjects::mpvplayer->setOptionGroup(group))
        {
            Notifier::getNotifier()->showMessage(Notifier::PLAYER_NOTIFY, QObject::tr("Switch to option group \"%1\"").arg(GlobalObjects::mpvplayer->currentOptionGroup()));
        }
    }, Qt::BlockingQueuedConnection);
    return 0;
}

}
