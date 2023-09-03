#include "resourcescript.h"
#include "Common/threadtask.h"

ResourceScript::ResourceScript() : ScriptBase()
{
    sType = ScriptType::RESOURCE;
    settingPath += "resource/";
}

ScriptState ResourceScript::loadScript(const QString &scriptPath)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo = ScriptBase::loadScript(scriptPath);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    hasDetailFunc = checkType(detailFunc, LUA_TFUNCTION);
    bool hasSearchFunc = checkType(searchFunc, LUA_TFUNCTION);
    if(!hasSearchFunc) return ScriptState(ScriptState::S_ERROR, "Search function is not found");
    return ScriptState(ScriptState::S_NORM);
}

ScriptState ResourceScript::search(const QString &keyword, int page, int &totalPage, QList<ResourceItem> &results, const QString &scene, const QMap<QString, QString> *option)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList params{keyword, page, scene};
    if(!option)
    {
        addSearchOptions(params);
    }
    else
    {
        QVariantMap optionMap;
        for(auto iter = option->cbegin(); iter != option->cend(); ++iter)
        {

            optionMap[iter.key()] = iter.value();
        }
        params.append(optionMap);
    }
    QVariantList rets = call(searchFunc, params, 2, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::List || !rets[1].canConvert(QVariant::Int)) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    totalPage = rets[1].toInt();
    if(totalPage <= 0) totalPage = 1;
    auto robjs = rets[0].toList(); //[{title=xx, <magnet=xx>, <time=xx>, <size=xx>, <url=xx>},...]
    for(auto &r : robjs)
    {
        auto robj = r.toMap();
        QString title = robj.value("title").toString();
        if(title.isEmpty()) continue;
        results.append({title, robj.value("time").toString(), robj.value("size").toString(), robj.value("magnet").toString(), robj.value("url").toString()});
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState ResourceScript::getDetail(const ResourceItem &oldItem, ResourceItem &newItem, const QString &scene)
{
    if(!hasDetailFunc) return ScriptState(ScriptState::S_ERROR, "No getdetail Function");
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(detailFunc, {oldItem.toMap(), scene}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type() != QVariant::Map) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto robj = rets[0].toMap();
    newItem = {
        robj.value("title").toString(),
        robj.value("time").toString(),
        robj.value("size").toString(),
        robj.value("magnet").toString(),
        robj.value("url").toString()
    };
    return ScriptState(ScriptState::S_NORM);
}
