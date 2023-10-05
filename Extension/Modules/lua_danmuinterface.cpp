#include "lua_danmuinterface.h"
#include "Extension/Common/ext_common.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "Play/Danmu/Manager/pool.h"
#include "Play/Danmu/danmupool.h"
#include "globalobjects.h"

namespace Extension
{

void DanmuInterface::setup()
{
    const luaL_Reg funcs[] = {
        {"launch", launch},
        {"getpool", getpool},
        {"addpool", addpool},
        {"getdanmu", getdanmu},
        {"addsrc", addsrc},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "danmu"}, funcs);
}

int DanmuInterface::launch(lua_State *L)
{
    if (lua_gettop(L) != 1) return 0;
    QList<QSharedPointer<DanmuComment>> danmuList;
    const QVariantList dobjs = getValue(L, true, 4).toList();
    for(auto &d : dobjs)
    {
        auto dobj = d.toMap();
        DanmuComment *comment = parseDanmu(dobj);
        if (comment) danmuList.append(QSharedPointer<DanmuComment>(comment));
    }
    QMetaObject::invokeMethod(GlobalObjects::danmuPool, [&](){
        GlobalObjects::danmuPool->launch(danmuList);
    }, Qt::BlockingQueuedConnection);
    return 0;
}

int DanmuInterface::getpool(lua_State *L)
{
    if (lua_gettop(L) != 1) return 0;
    Pool *pool = nullptr;
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        const QString poolId = lua_tostring(L, 1);
        pool = GlobalObjects::danmuManager->getPool(poolId, false);
    }
    else if (lua_type(L, 1) == LUA_TTABLE)
    {
        const QVariantMap params = getValue(L, true, 2).toMap();
        bool loadDanmu = params.value("load", false).toBool();
        if (params.contains("id"))
        {
            const QString poolId = params.value("id").toString();
            pool = GlobalObjects::danmuManager->getPool(poolId, loadDanmu);
        }
        else
        {
            if (params.contains("anime") && params.contains("ep_index") && params.contains("ep_type"))
            {
                const QString anime = params.value("anime").toString();
                double epIndex = params.value("ep_index").toDouble();
                EpType epType = static_cast<EpType>(params.value("ep_type").toInt());
                pool = GlobalObjects::danmuManager->getPool(anime, epType, epIndex, loadDanmu);
            }
        }
    }
    if (!pool) return 0;
    pushValue(L, pool->toMap());
    return 1;
}

int DanmuInterface::addpool(lua_State *L)
{
    if (lua_gettop(L) != 1) return 0;
    const QVariantMap params = getValue(L, true, 2).toMap();
    const QString anime = params.value("anime").toString();
    double epIndex = params.value("ep_index", -1).toDouble();
    EpType epType = static_cast<EpType>(params.value("ep_type", 0).toInt());
    const QString epName = params.value("ep_name").toString();
    if (anime.isEmpty() || epIndex < 0 || epType == EpType::UNKNOWN)
    {
        return 0;
    }
    const QString pid = GlobalObjects::danmuManager->createPool(anime, epType, epIndex, epName);
    lua_pushstring(L, pid.toUtf8().constData());
    return 1;
}

int DanmuInterface::getdanmu(lua_State *L)
{
    if (lua_gettop(L) != 1) return 0;
    const QString poolId = lua_tostring(L, 1);
    Pool *pool = GlobalObjects::danmuManager->getPool(poolId, true);
    if (!pool) return 0;
    pushValue(L, pool->exportFullJson().toVariantMap());
    return 1;
}

int DanmuInterface::addsrc(lua_State *L)
{
    if (lua_gettop(L) != 2) return 0;
    const QString poolId = lua_tostring(L, 1);
    Pool *pool = GlobalObjects::danmuManager->getPool(poolId);
    if (!pool) return 0;

    const QVariantMap srcInfo = getValue(L, true, 5).toMap();
    if (!srcInfo.contains("source")) return 0;
    const QVariantMap src = srcInfo["source"].toMap();
    if (!src.contains("name") || !src.contains("scriptId") || !src.contains("scriptData")) return 0;
    DanmuSource sourceInfo;
    sourceInfo.title = src["name"].toString();
    sourceInfo.scriptId = src["scriptId"].toString();
    sourceInfo.scriptData = src["scriptData"].toString();
    sourceInfo.delay = src["delay"].toInt();
    sourceInfo.duration = src["duration"].toInt();
    sourceInfo.desc = src["desc"].toString();
    if (src.contains("timeline")) sourceInfo.setTimeline(src["timeline"].toString());
    if (sourceInfo.title.isEmpty() || sourceInfo.scriptId.isEmpty() || sourceInfo.scriptData.isEmpty()) return 0;

    QVector<DanmuComment *> danmuList;
    const QVariantList dobjs = srcInfo["comment"].toList();
    for(auto &d : dobjs)
    {
        auto dobj = d.toMap();
        DanmuComment *comment = parseDanmu(dobj);
        if (comment) danmuList.append(comment);
    }
    bool saveSrc = srcInfo.value("save", true).toBool();
    int srcId = pool->addSource(sourceInfo, danmuList, saveSrc);
    if (srcId < 0) qDeleteAll(danmuList);
    lua_pushinteger(L, srcId);
    return 1;
}

DanmuComment *DanmuInterface::parseDanmu(const QVariantMap &dobj)
{
    // [{text=xx, time=xx(number, ms), <color=xx(int)>, <fontsize=xx(int, 1=normal, 2=small, 3=large)> <type=xx(int, 0=roll,1=top,2=bottom)>, <date=xx(str)>, <sender=xx>},....]
    const QString text = dobj.value("text").toString();
    double time = dobj.value("time", -1).toDouble();
    if(text.isEmpty() || time<0)  return nullptr;
    DanmuComment *comment = new DanmuComment;
    comment->text = text;
    comment->time = comment->originTime = time;
    comment->color = dobj.value("color", 0xFFFFFF).toInt();
    int fontsize = dobj.value("fontsize", 0).toInt();
    comment->fontSizeLevel = (fontsize == 1? DanmuComment::Small:(fontsize == 2? DanmuComment::Large : DanmuComment::Normal));
    int type = dobj.value("type", (int)DanmuComment::Rolling).toInt();
    comment->type = (DanmuComment::DanmuType)type;
    comment->date = dobj.value("date", 0).toLongLong();
    comment->sender = dobj.value("sender").toString();
    return comment;
}

}
