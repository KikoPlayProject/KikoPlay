#include "danmuprovider.h"
#include "Common/threadtask.h"
#include "Common/network.h"
#include "Extension/Script/scriptmanager.h"
#include "Extension/Script/danmuscript.h"
#include "Manager/danmumanager.h"
#include "Manager/pool.h"
#include "globalobjects.h"
#ifdef KSERVICE
#include "Service/kservice.h"
#endif

DanmuProvider::DanmuProvider(QObject *parent) : QObject(parent)
{

}

QList<QPair<QString, QString>> DanmuProvider::getSearchProviders()
{
    QList<QPair<QString, QString>> searchProviders;
    for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::DANMU))
    {
        DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
        if(dmScript->supportSearch())
        {
            searchProviders.append({dmScript->name(), dmScript->id()});
        }
    }
    return searchProviders;
}

QList<QPair<QString, QStringList>> DanmuProvider::getSampleURLs()
{
    QList<QPair<QString, QStringList>> sampledURLs;
    for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::DANMU))
    {
        DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
        if (dmScript->sampleURLs().isEmpty()) continue;
        sampledURLs.append({dmScript->name(), dmScript->sampleURLs()});
    }
    return sampledURLs;
}

ScriptState DanmuProvider::danmuSearch(const QString &scriptId, const QString &keyword, const QMap<QString, QString> &options, QList<DanmuSource> &results, TaskContext *ctx)
{
    auto script = GlobalObjects::scriptManager->getScript(scriptId).staticCast<DanmuScript>();
    if(!script || !script->supportSearch()) return "Script invalid or Unsupport search";
    ThreadTask task(GlobalObjects::workThread);
    Network::ReqAbortFlagObj *abortFlag = nullptr;
    if (ctx)
    {
        QObject::connect(ctx, &TaskContext::cancelRequested, this, [&](){
            if (abortFlag) emit abortFlag->abort();
            if (script) script->stop();
        });
    }
    return task.Run([&](){
        abortFlag = Network::getAbortFlag();
        for(auto iter = options.cbegin(); iter != options.cend(); ++iter)
        {
            script->setSearchOption(iter.key(), iter.value());
        }
        return QVariant::fromValue(script->search(keyword, results));
    }).value<ScriptState>();
}

ScriptState DanmuProvider::getEpInfo(const DanmuSource *source, QList<DanmuSource> &results, TaskContext *ctx)
{
    auto script = GlobalObjects::scriptManager->getScript(source->scriptId).staticCast<DanmuScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    Network::ReqAbortFlagObj *abortFlag = nullptr;
    if (ctx)
    {
        QObject::connect(ctx, &TaskContext::cancelRequested, this, [&](){
            if (abortFlag) emit abortFlag->abort();
            if (script) script->stop();
        });
    }
    return task.Run([&](){
        abortFlag = Network::getAbortFlag();
        return QVariant::fromValue(script->getEpInfo(source, results));
    }).value<ScriptState>();
}

ScriptState DanmuProvider::getURLInfo(const QString &url, QList<DanmuSource> &results, TaskContext *ctx)
{
    ThreadTask task(GlobalObjects::workThread);
    Network::ReqAbortFlagObj *abortFlag = nullptr;
    DanmuScript *_script = nullptr;
    bool cancelFlag = false;
    if (ctx)
    {
        QObject::connect(ctx, &TaskContext::cancelRequested, this, [&](){
            cancelFlag = true;
            if (abortFlag) emit abortFlag->abort();
            if (_script) _script->stop();
        });
    }
    return task.Run([&](){
        abortFlag = Network::getAbortFlag();
        for(auto &script : GlobalObjects::scriptManager->scripts(ScriptType::DANMU))
        {
            if (cancelFlag) break;
            DanmuScript *dmScript = static_cast<DanmuScript *>(script.data());
            if(dmScript->supportURL(url))
            {
                _script = dmScript;
                return QVariant::fromValue(dmScript->getURLInfo(url, results));
            }
        }
        return QVariant::fromValue(ScriptState(ScriptState::S_ERROR, "Unsupported URL"));
    }).value<ScriptState>();
}

ScriptState DanmuProvider::downloadDanmu(const DanmuSource *item, QVector<DanmuComment *> &danmuList, DanmuSource **nItem, TaskContext *ctx)
{
#ifdef KSERVICE
    if (item->isKikoSource())
    {
        KService::instance()->getDanmu(*item);
        return ScriptState(ScriptState::S_NORM);
    }
#endif
    auto script = GlobalObjects::scriptManager->getScript(item->scriptId).staticCast<DanmuScript>();
    if(!script) return "Script invalid";
    ThreadTask task(GlobalObjects::workThread);
    Network::ReqAbortFlagObj *abortFlag = nullptr;
    if (ctx)
    {
        QObject::connect(ctx, &TaskContext::cancelRequested, this, [&](){
            if (abortFlag) emit abortFlag->abort();
            if (script) script->stop();
        });
    }
    return task.Run([&](){
        abortFlag = Network::getAbortFlag();
        DanmuSource *retItem = nullptr;
        ScriptState state = script->getDanmu(item, &retItem, danmuList);
        if(nItem) *nItem = retItem;
        else if (retItem) delete retItem;
        return QVariant::fromValue(state);
    }).value<ScriptState>();
}

