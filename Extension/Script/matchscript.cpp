#include "matchscript.h"

MatchScript::MatchScript() : ScriptBase()
{
    sType = ScriptType::MATCH;
    settingPath += "match/";
}

ScriptState MatchScript::loadScript(const QString &scriptPath)
{
    MutexLocker locker(scriptLock);
    if (!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo = ScriptBase::loadScript(scriptPath);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    bool hasMatchFunc = checkType(matchFunc, LUA_TFUNCTION);
    if (!hasMatchFunc) return ScriptState(ScriptState::S_ERROR, "match function is not found");
    return ScriptState(ScriptState::S_NORM);
}

ScriptState MatchScript::match(const QString &path, MatchResult &result)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(matchFunc, {path}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].metaType().id() != QMetaType::QVariantMap) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto m = rets[0].toMap(); //{success=xx, anime={name=xx,data=xx}, ep={name=xx, index=number, <type=xx>}}
    do
    {
        if (!m.value("success", false).toBool()) break;
        if (!m.contains("anime") || !m.contains("ep")) break;
        auto anime = m.value("anime"), ep = m.value("ep");
        if (anime.metaType().id() != QMetaType::QVariantMap || ep.metaType().id() != QMetaType::QVariantMap) break;
        auto aobj = anime.toMap(), epobj = ep.toMap();
        QString animeName = aobj.value("name").toString();
        QString scriptData = aobj.value("data").toString();
        QString scriptId = aobj.value("scriptId").toString();
        QString epName = epobj.value("name").toString();
        double epIndex = epobj.value("index", -1).toDouble();
        if (animeName.isEmpty() || epName.isEmpty() || epIndex < 0) break;
        result.success = true;
        result.name = animeName;
        if (!scriptData.isEmpty() && !scriptId.isEmpty())
        {
            result.scriptId = scriptId;
            result.scriptData = scriptData;
        }
        EpInfo &epinfo = result.ep;
        epinfo.localFile = path;
        epinfo.index = epIndex; epinfo.name = epName;
        epinfo.type = EpType(qBound(1, epobj.value("type", 1).toInt(), int(EpType::Other)));
        return ScriptState(ScriptState::S_NORM);
    } while (false);
    result.success = false;
    return ScriptState(ScriptState::S_NORM);
}
