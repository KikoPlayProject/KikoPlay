#include "danmuscript.h"
#include <QRegularExpression>

DanmuScript::DanmuScript() : ScriptBase()
{
    sType = ScriptType::DANMU;
    settingPath += "danmu/";
}

ScriptState DanmuScript::loadScript(const QString &scriptPath)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo = ScriptBase::loadScript(scriptPath);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    canSearch = checkType("search", LUA_TFUNCTION);
    canLaunch = checkType("launch", LUA_TFUNCTION);
    QVariant res = get(urlReTable);
    if(res.canConvert(QVariant::StringList))
    {
        supportedURLsRe = res.toStringList();
    }
    QVariant urlSamples = get(sampleUrlTable);
    if(urlSamples.canConvert(QVariant::StringList))
    {
        sampleSupporedURLs = urlSamples.toStringList();
    }
    return ScriptState(ScriptState::S_NORM);
}

QColor DanmuScript::labelColor() const
{
    QColor defaultColor{43, 106, 176};
    if (scriptMeta.contains(labelColorName))
    {
        bool ok = false;
        int val = scriptMeta[labelColorName].toInt(&ok, 16);
        if (ok)
        {
            defaultColor = QColor((val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
        }
    }
    return defaultColor;
}

ScriptState DanmuScript::search(const QString &keyword, QList<DanmuSource> &results)
{
    if(!canSearch) return ScriptState(ScriptState::S_ERROR, "Search not supported");
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo(callGetSources(luaSearchFunc, keyword, true, results));
    return ScriptState(errInfo.isEmpty()?ScriptState::S_NORM:ScriptState::S_ERROR, errInfo);
}

ScriptState DanmuScript::getEpInfo(const DanmuSource *source, QList<DanmuSource> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo(callGetSources(luaEpFunc, source->toMap(), false, results));
    return ScriptState(errInfo.isEmpty()?ScriptState::S_NORM:ScriptState::S_ERROR, errInfo);
}

ScriptState DanmuScript::getURLInfo(const QString &url, QList<DanmuSource> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo(callGetSources(luaURLFunc, url, false, results));
    return ScriptState(errInfo.isEmpty()?ScriptState::S_NORM:ScriptState::S_ERROR, errInfo);
}

ScriptState DanmuScript::getDanmu(const DanmuSource *item, DanmuSource **nItem, QVector<DanmuComment *> &danmuList)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(luaDanmuFunc, {item->toMap()}, 2, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if((rets[0].type()!=QVariant::Map && rets[0].type()!=QVariant::Invalid) || rets[1].type()!=QVariant::List) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    if(rets[0].type() == QVariant::Invalid)
    {
        *nItem = nullptr;
    }
    else if(rets[0].type() == QVariant::Map)
    {
        DanmuSource *nSrc = new DanmuSource;
        auto itemObj = rets[0].toMap();
        *nSrc = *item;
        nSrc->title = itemObj.value("title").toString();
        nSrc->desc = itemObj.value("desc").toString();
        nSrc->scriptData = itemObj.value("data").toString();
        nSrc->duration = itemObj.value("duration", 0).toInt();
        *nItem = nSrc;
    }
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
        int fontsize = dobj.value("fontsize", 0).toInt();
        comment->fontSizeLevel = (fontsize == 1? DanmuComment::Small:(fontsize == 2? DanmuComment::Large : DanmuComment::Normal));
        int type = dobj.value("type", (int)DanmuComment::Rolling).toInt();
        comment->type = (DanmuComment::DanmuType)type;
        comment->date = dobj.value("date", 0).toLongLong();
        comment->sender = dobj.value("sender").toString();
        danmuList.append(comment);
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState DanmuScript::hasSourceToLaunch(const QList<DanmuSource> &sources, bool &result)
{
    if(!canLaunch)
    {
        result = false;
        return ScriptState(ScriptState::S_NORM);
    }
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QVariantList params;
    for(auto &src: sources)
        params.append(src.toMap());
    QString errInfo;
    QVariantList param;
    param.append(QVariant(params));
    QVariantList rets = call(luaLaunchCheckFunc, param, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::Bool) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    result = rets[0].toBool();
    return ScriptState(ScriptState::S_NORM);
}

ScriptState DanmuScript::launch(const QList<DanmuSource> &sources, const DanmuComment *comment)
{
    if(!comment) return ScriptState(ScriptState::S_NORM);
    if(!canLaunch) return ScriptState(ScriptState::S_ERROR, "Not Support: Launch");
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QVariantList srcs;
    for(auto &src: sources)
        srcs.append(src.toMap());
    QString errInfo;
    QVariantList rets = call(luaLaunchFunc, {srcs, comment->toMap()}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()==QVariant::String) return ScriptState(ScriptState::S_ERROR, rets[0].toString());
    return ScriptState(ScriptState::S_NORM);
}

bool DanmuScript::supportURL(const QString &url)
{
    bool supported = false;
    for(const QString &s : supportedURLsRe)
    {
        QRegularExpression re(s);
        QRegularExpressionMatch match = re.match(url);
        if(match.capturedLength(0) == url.length())
        {
            supported = true;
            break;
        }
    }
    return supported;
}

QString DanmuScript::callGetSources(const char *fname, const QVariant &param, bool passOption, QList<DanmuSource> &results)
{
    QString errInfo;
    QVariantList params{param};
    if(passOption) addSearchOptions(params);
    QVariantList rets = call(fname, params, 1, errInfo);
    if(!errInfo.isEmpty()) return errInfo;
    QVariant ret = rets.first();  // array, [{title='', <desc>='', data='', <delay>=xx, <duration>=''},{}...]
    if(ret.type() != QVariant::List) return "Wrong Return Value Type";
    QVariantList sourceList = ret.toList();
    results.clear();
    for(auto &item : sourceList)
    {
        if(item.type() == QVariant::Map)
        {
            auto itemObj = item.toMap();
            QString title(itemObj.value("title").toString());
            QString data(itemObj.value("data").toString());
            if(title.isEmpty() || data.isEmpty()) continue;
            DanmuSource src;
            src.title = title;
            src.desc = itemObj.value("desc").toString();
            src.scriptData = data;
            src.scriptId = id();
            src.delay = itemObj.value("delay", 0).toInt();
            src.duration = itemObj.value("duration", 0).toInt();
            results.append(src);
        }
    }
    return errInfo;
}
