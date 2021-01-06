#include "danmuscript.h"

DanmuScript::DanmuScript() : ScriptBase()
{

}

QString DanmuScript::load(const QString &scriptPath)
{
    QString errInfo = loadScript(scriptPath);
    if(!errInfo.isEmpty()) return errInfo;
    canSearch = checkType("search", LUA_TFUNCTION);
    if(!scriptMeta.contains("id"))
    {
        scriptMeta["id"] = QFileInfo(scriptPath).baseName();
    }
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
    return errInfo;
}

QString DanmuScript::search(const QString &keyword, QList<DanmuSource> &results)
{
    if(!canSearch) return "Script Error: Search not supported";
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return "";
    QString errInfo;
    QVariantList rets = call("search", {keyword}, 1, errInfo);
    if(!errInfo.isEmpty()) return errInfo;
    QVariant ret = rets.first();  // array, [{title='', <desc>='', idinfo='', <delay>=xx, <count>=''},{}...]
    if(ret.type() != QVariant::List) return "Script Error: Wrong return values";
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
