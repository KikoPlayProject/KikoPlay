#include "libraryscript.h"
#include "Common/network.h"
#include <QImageReader>

LibraryScript::LibraryScript() : ScriptBase()
{
    sType = ScriptType::LIBRARY;
    settingPath += "library/";
}

ScriptState LibraryScript::loadScript(const QString &scriptPath)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo = ScriptBase::loadScript(scriptPath);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    matchSupported = checkType(matchFunc, LUA_TFUNCTION);
    bool hasSearchFunc = checkType(searchFunc, LUA_TFUNCTION);
    bool hasEpFunc = checkType(epFunc, LUA_TFUNCTION);
    hasTagFunc = checkType(tagFunc, LUA_TFUNCTION);
    if(!matchSupported && !(hasSearchFunc && hasEpFunc)) return ScriptState(ScriptState::S_ERROR, "search+getep/match function is not found");
    QVariant menus = get(menusTable); //[{title=xx, id=xx}...]
    if(menus.type() == QVariant::List)
    {
        auto mlist = menus.toList();
        for(auto &m : mlist)
        {
            if(m.type() == QVariant::Map)
            {
                auto mObj = m.toMap();
                QString title = mObj.value("title").toString(), id =mObj.value("id").toString();
                if(title.isEmpty() || id.isEmpty()) continue;
                menuItems.append({title, id});
            }
        }
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::search(const QString &keyword, QList<AnimeLite> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList params{keyword};
    addSearchOptions(params);
    QVariantList rets = call(searchFunc, params, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::List) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto robjs = rets[0].toList(); //[{name=xx, data=xx, <extra=xx>, <eps=[{index=number, name=xx, <type=xx(1:ep, 2:sp, ...,7=other)>, }]>},...]
    for(auto &r : robjs)
    {
        auto robj = r.toMap();
        QString name = robj.value("name").toString(), scriptData = robj.value("data").toString();
        if(name.isEmpty()) continue;
        AnimeLite ab;
        ab.name = name;
        ab.extras = robj.value("extra").toString();
        ab.scriptId = robj.contains("scriptId")?robj.value("scriptId").toString():id();
        ab.scriptData = robj.value("data").toString();
        if(robj.contains("eps") && robj.value("eps").type()==QVariant::List)
        {
            QVector<EpInfo> *epList = new QVector<EpInfo>;
            auto eps = robj.value("eps").toList();
            QMap<EpType, QPair<double, QSet<double>>> reOrderMap;
            for(auto e: eps)
            {
                auto epobj = e.toMap();
                double index = epobj.value("index", -1).toDouble();
                QString epName = epobj.value("name").toString();
                if(index<0) continue;
                EpInfo ep;
                ep.name = epName; ep.index = index;
                ep.type = EpType(qBound(1, epobj.value("type", 1).toInt(), int(EpType::Other)));
                if(!reOrderMap.contains(ep.type))
                {
                    reOrderMap.insert(ep.type, {1, QSet<double>()});
                }
                if(reOrderMap[ep.type].second.contains(ep.index))
                {
                    ep.index = reOrderMap[ep.type].first + 1;
                }
                reOrderMap[ep.type].first = qMax(reOrderMap[ep.type].first, ep.index);
                reOrderMap[ep.type].second.insert(ep.index);
                epList->append(ep);
            }
            std::sort(epList->begin(), epList->end());
            ab.epList.reset(epList);
        }
        results.append(ab);
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::getDetail(const AnimeLite &base, Anime *anime)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(detailFunc, {base.toMap()}, 1, errInfo, false);
    if (!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if (rets[0].metaType().id() != QMetaType::QVariantMap) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    // {name=xx, <desc=xx>, airdate=xx(yyyy-mm-dd), data=xx, <epcount=xx(int)>, <coverurl=xx>, <cover=xx> <url=xx>,
    //  <staff=xx("xx:xx;yy:yy;...")>, <crt=[{name=xx,<actor=xx>,<link=xx>, <imgurl=xx>, <image=xx>},...]>>}
    auto aobj = rets[0].toMap();
    QString name = aobj.value("name").toByteArray(),
            scriptData=aobj.value("data").toByteArray(),
            airdate=aobj.value("airdate").toByteArray();
    if (name.isEmpty() || airdate.isEmpty()) return ScriptState(ScriptState::S_ERROR, "Wrong anime info");
    anime->_name = name;
    anime->_desc = aobj.value("desc").toByteArray();
    anime->_url = aobj.value("url").toByteArray();
    anime->_scriptId = id();
    anime->_scriptData = scriptData;
    anime->_airDate = airdate;
    anime->_epCount = aobj.value("epcount", 0).toInt();
    anime->_coverURL = aobj.value("coverurl").toByteArray();
    if (aobj.contains("cover"))
    {
        anime->_coverData = aobj.value("cover").toByteArray();
    }
    else if (!anime->_coverURL.isEmpty())
    {
        Network::Reply reply(Network::httpGet(anime->coverURL(), QUrlQuery()));
        if (!reply.hasError)
        {
            anime->_coverData = reply.content;
        }
    }
    anime->staff.clear();
    if (aobj.contains("staff"))
    {
        QString staffstr = aobj.value("staff").toString();
        auto strs = staffstr.split(';', Qt::SkipEmptyParts);
        for(const auto &s : strs)
        {
            int pos = s.indexOf(':');
            anime->staff.append({s.mid(0, pos), s.mid(pos+1)});
        }
    }
    anime->characters.clear();
    if (aobj.contains("crt") && aobj.value("crt").metaType().id() == QMetaType::QVariantList)
    {
        auto crts = aobj.value("crt").toList();
        auto crtImgLoad = [](QByteArray &data, Character *crt){
            QBuffer bufferImage(&data);
            bufferImage.open(QIODevice::ReadOnly);
            QImageReader reader(&bufferImage);
            QSize s = reader.size();
            int w = qMin(s.width(), s.height());
            reader.setScaledClipRect(QRect(0, 0, w, w));
            crt->image = QPixmap::fromImageReader(&reader);
            Character::scale(crt->image);
        };

        QStringList urls;
        QList<QUrlQuery> querys;
        QList<int> crtIndexes;

        for (auto &c : crts)
        {
            if(c.metaType().id() != QMetaType::QVariantMap) continue;
            auto cobj = c.toMap();
            QString cname = cobj.value("name").toString();
            if(cname.isEmpty()) continue;
            Character crt;
            crt.name = cname;
            crt.actor = cobj.value("actor").toString();
            crt.link = cobj.value("link").toString();
            crt.imgURL = cobj.value("imgurl").toString();
            if (cobj.contains("image"))
            {
                QByteArray imgData = cobj.value("image").toByteArray();
                crtImgLoad(imgData, &crt);
            }
            else if (!crt.imgURL.isEmpty())
            {
                urls.append(crt.imgURL);
                querys.append(QUrlQuery());
                crtIndexes.append(anime->characters.size());
            }
            anime->characters.append(crt);
        }

        if (!urls.empty())
        {
            QList<Network::Reply> results{Network::httpGetBatch(urls,querys)};
            for (int i = 0; i < crtIndexes.size(); ++i)
            {
                if (results[i].hasError) continue;
                crtImgLoad(results[i].content, &(anime->characters[crtIndexes[i]]));
            }
        }
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::getEp(Anime *anime, QVector<EpInfo> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(epFunc, {anime->toMap()}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::List) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto eps = rets[0].toList();
    QMap<EpType, QPair<double, QSet<double>>> reOrderMap;
    for(auto e: eps)
    {
        auto epobj = e.toMap();
        double index = epobj.value("index", 0).toDouble();
        QString epName = epobj.value("name").toString();
        if(index<0) continue;
        EpInfo ep;
        ep.name = epName; ep.index = index;
        ep.type = EpType(qBound(1, epobj.value("type", 1).toInt(), int(EpType::Other)));
        if(!reOrderMap.contains(ep.type))
        {
            reOrderMap.insert(ep.type, {1, QSet<double>()});
        }
        if(reOrderMap[ep.type].second.contains(ep.index))
        {
            ep.index = reOrderMap[ep.type].first + 1;
        }
        reOrderMap[ep.type].first = qMax(reOrderMap[ep.type].first, ep.index);
        reOrderMap[ep.type].second.insert(ep.index);
        results.append(ep);
    }
	std::sort(results.begin(), results.end());
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::getTags(Anime *anime, QStringList &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(tagFunc, {anime->toMap()}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(!rets[0].canConvert(QVariant::StringList)) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    QSet<QString> tags;
    auto list = rets[0].toStringList();
    for(const QString &tag : list)
    {
        if(!tag.trimmed().isEmpty())
        {
            tags.insert(tag.trimmed());
        }
    }
    results = QStringList(tags.begin(), tags.end());
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::match(const QString &path, MatchResult &result)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call(matchFunc, {path}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::Map) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto m = rets[0].toMap(); //{success=xx, anime={name=xx,data=xx}, ep={name=xx, index=number, <type=xx>}}
    do
    {
        if(!m.value("success", false).toBool()) break;
        if(!m.contains("anime") || !m.contains("ep")) break;
        auto anime = m.value("anime"), ep = m.value("ep");
        if(anime.type()!=QVariant::Map || ep.type()!= QVariant::Map) break;
        auto aobj = anime.toMap(), epobj = ep.toMap();
        QString animeName = aobj.value("name").toString(), animeId = aobj.value("data").toString();
        QString epName = epobj.value("name").toString();
        double epIndex = epobj.value("index", -1).toDouble();
        if(animeName.isEmpty() || animeId.isEmpty() || epName.isEmpty() || epIndex<0) break;
        result.success = true;
        result.name = animeName;
        result.scriptId = aobj.contains("scriptId")?aobj.value("scriptId").toString():id();
        result.scriptData = animeId;
        EpInfo &epinfo = result.ep;
        epinfo.localFile = path;
        epinfo.index = epIndex; epinfo.name = epName;
        epinfo.type = EpType(qBound(1, epobj.value("type", 1).toInt(), int(EpType::Other)));
        return ScriptState(ScriptState::S_NORM);
    }while(false);
    result.success = false;
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::menuClick(const QString &mid, Anime *anime)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    call(menuFunc, {mid, anime->toMap(true)}, 0, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    return ScriptState(ScriptState::S_NORM);
}

