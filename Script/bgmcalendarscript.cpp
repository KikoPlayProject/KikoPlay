#include "bgmcalendarscript.h"
#include <QVariant>

BgmCalendarScript::BgmCalendarScript() : ScriptBase()
{
    sType = ScriptType::BGM_CALENDAR;
    settingPath += "bgm_calendar/";
}

ScriptState BgmCalendarScript::loadScript(const QString &scriptPath)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo = ScriptBase::loadScript(scriptPath);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(!checkType(seasonFunc, LUA_TFUNCTION))
        return ScriptState(ScriptState::S_ERROR, QString("\"%1\" function is not found").arg(seasonFunc));
    if(!checkType(bgmlistFunc, LUA_TFUNCTION))
        return ScriptState(ScriptState::S_ERROR, QString("\"%1\" function is not found").arg(bgmlistFunc));
    return ScriptState(ScriptState::S_NORM);
}

ScriptState BgmCalendarScript::getSeason(QList<BgmSeason> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(seasonFunc, {}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::List) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto robjs = rets[0].toList(); //[{title=xx, data=xx, },...]
    results.clear();
    for(auto &r : robjs)
    {
        auto robj = r.toMap();
        QString title = robj.value("title").toString();
        if(title.isEmpty()) continue;
        BgmSeason bs;
        bs.title = title;
        bs.data = robj.value("data").toString();
        results.append(bs);
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState BgmCalendarScript::getBgmList(BgmSeason &season)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(bgmlistFunc, {season.toMap()}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type() != QVariant::List) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto robjs = rets[0].toList(); //[{title=xx, weekday=xx, time=xx, date=xx, bgmid=xx, sites=[{name=xx, url=xx},..], isnew=t/f, focus=t/f]},...]
    season.bgmList.clear();
    season.newBgmCount = 0;
    for(auto &r : robjs)
    {
        auto robj = r.toMap();
        QString title = robj.value("title").toString();
        if(title.isEmpty()) continue;
        BgmItem item;
        item.title = title;
        item.weekDay = robj.value("weekday").toInt();
        item.airTime = robj.value("time").toString();
        item.airDate = robj.value("date").toString();
        item.bgmId = robj.value("bgmid").toString();
        item.isNew = robj.value("isnew", true).toBool();
        item.focus = false;
        season.newBgmCount += (item.isNew?1:0);
        if(robj.value("focus", false).toBool())
        {
            item.focus = true;
            season.focusSet.insert(title);
        }
        if(robj.contains("sites"))
        {
            auto sites = robj.value("sites").toList();
            for(auto s : sites)
            {
                auto sobj = s.toMap();
                item.onAirSites.append(sobj.value("name").toString());
                item.onAirURLs.append(sobj.value("url").toString());
            }
        }
        season.bgmList.append(item);
    }
    return ScriptState(ScriptState::S_NORM);
}
