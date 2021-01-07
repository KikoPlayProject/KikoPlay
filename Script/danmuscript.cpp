#include "danmuscript.h"

DanmuScript::DanmuScript() : ScriptBase()
{

}

ScriptState DanmuScript::load(const QString &scriptPath)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo = loadScript(scriptPath);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    canSearch = checkType("search", LUA_TFUNCTION);
    QVariant res = get("supportedURLsRe");
    if(res.type() == QVariant::StringList)
    {
        supportedURLsRe = res.toStringList();
    }
    QVariant urlSamples = get("sampleSupporedURLs");
    if(urlSamples.type() == QVariant::StringList)
    {
        sampleSupporedURLs = urlSamples.toStringList();
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState DanmuScript::search(const QString &keyword, QList<DanmuSource> &results)
{
    if(!canSearch) return ScriptState(ScriptState::S_ERROR, "Search not supported");
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo(callGetSources("search", keyword, results));
    return ScriptState(errInfo.isEmpty()?ScriptState::S_NORM:ScriptState::S_ERROR, errInfo);
}

ScriptState DanmuScript::getEpInfo(const DanmuSource *source, QList<DanmuSource> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo(callGetSources("epinfo", source->toMap(), results));
    return ScriptState(errInfo.isEmpty()?ScriptState::S_NORM:ScriptState::S_ERROR, errInfo);
}

ScriptState DanmuScript::getURLInfo(const QString &url, QList<DanmuSource> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo(callGetSources("urlinfo", url, results));
    return ScriptState(errInfo.isEmpty()?ScriptState::S_NORM:ScriptState::S_ERROR, errInfo);
}

ScriptState DanmuScript::getDanmu(const DanmuSource *item, DanmuSource &nItem, QList<DanmuComment *> &danmuList)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call("danmu", {item->toMap()}, 2, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::Map || rets[1].type()!=QVariant::List) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto itemObj = rets[0].toMap();
    QString title(itemObj.value("title").toString());
    QString idInfo(itemObj.value("idinfo").toString());
    nItem = {title, itemObj.value("desc").toString(), idInfo, id(),
                    itemObj.value("delay", 0).toInt(),
                    itemObj.value("count", 0).toInt()};
    auto dobjs = rets[1].toList();  //[{text=xx, time=xx(number, ms), <color=xx(int)>, <fontsize=xx(int, 1=normal, 2=small, 3=large)> <type=xx(int, 1=roll,2=top,3=bottom)>, <date=xx(str)>, <sender=xx>},....]
    for(auto &d : dobjs)
    {
        auto dobj = d.toMap();
        QString text = dobj.value("text").toString();
        double time = dobj.value("time", -1).toDouble();
        if(text.isEmpty() || time<0) continue;
        DanmuComment *comment = new DanmuComment;
        comment->text = text;
        comment->time = comment->originTime = time;
        comment->color = dobj.value("color", 0xFFFFFF).toInt();
        int fontsize = dobj.value("fontsize", 1).toInt();
        comment->fontSizeLevel = (fontsize == 2? DanmuComment::Small:(fontsize == 3? DanmuComment::Large : DanmuComment::Normal));
        int type = dobj.value("type", 1).toInt();
        comment->type = (type == 2? DanmuComment::Top:(type == 3? DanmuComment::Bottom : DanmuComment::Rolling));
        comment->date = dobj.value("date", 0).toLongLong();
        comment->sender = dobj.value("sender").toString();
        danmuList.append(comment);
    }
    return ScriptState(ScriptState::S_NORM);
}

QString DanmuScript::callGetSources(const char *fname, const QVariant &param, QList<DanmuSource> &results)
{
    QString errInfo;
    QVariantList rets = call(fname, {param}, 1, errInfo);
    if(!errInfo.isEmpty()) return errInfo;
    QVariant ret = rets.first();  // array, [{title='', <desc>='', idinfo='', <delay>=xx, <count>=''},{}...]
    if(ret.type() != QVariant::List) return "Wrong Return Value Type";
    QVariantList sourceList = ret.toList();
    results.clear();
    for(auto &item : sourceList)
    {
        if(item.type() == QVariant::Map)
        {
            auto itemObj = item.toMap();
            QString title(itemObj.value("title").toString());
            QString idInfo(itemObj.value("idinfo").toString());
            if(title.isEmpty() || idInfo.isEmpty()) continue;
            results.append({title, itemObj.value("desc").toString(), idInfo, id(),
                            itemObj.value("delay", 0).toInt(),
                            itemObj.value("count", 0).toInt()});
        }
    }
    return errInfo;
}
