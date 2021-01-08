#include "libraryscript.h"

LibraryScript::LibraryScript() : ScriptBase()
{

}

ScriptState LibraryScript::load(const QString &scriptPath)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo = loadScript(scriptPath);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    matchSupported = checkType("match", LUA_TFUNCTION);
    bool hasSearchFunc = checkType("search", LUA_TFUNCTION);
    bool hasEpFunc = checkType("getep", LUA_TFUNCTION);
    hasTagFunc = checkType("gettags", LUA_TFUNCTION);
    if(!matchSupported || !(hasSearchFunc && hasEpFunc)) return ScriptState(ScriptState::S_ERROR, "search+getep/match function is not found");
    QVariant menus = get("menus"); //[{title=xx, id=xx}...]
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

ScriptState LibraryScript::search(const QString &keyword, QList<AnimeBase> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call("search", {keyword}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::List) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto robjs = rets[0].toList(); //[{name=xx, id=xx, <eps=[{index=number, name=xx, <type=xx(1:ep, 2:sp, ...,7=other)>, }]>},...]
    for(auto &r : robjs)
    {
        auto robj = r.toMap();
        QString name = robj.value("name").toString(), id = robj.value("id").toString();
        if(name.isEmpty() or id.isEmpty()) continue;
        AnimeBase ab;
        ab.name = name; ab.id = id;
        if(robj.contains("eps") && robj.value("eps").type()==QVariant::List)
        {
            QList<EpInfo> *epList = new QList<EpInfo>;
            auto eps = robj.value("eps").toList();
            for(auto e: eps)
            {
                auto epobj = e.toMap();
                double index = epobj.value("index", -1).toDouble();
                QString epName = epobj.value("name").toString();
                if(index<0 || epName.isEmpty()) continue;
                EpInfo ep;
                ep.name = epName; ep.index = index;
                ep.type = EpInfo::EpType(qBound(1, epobj.value("type", 1).toInt(), int(EpInfo::EpType::Other)));
                epList->append(ep);
            }
            ab.epList.reset(epList);
        }
        results.append(ab);
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::getDetail(const QString &id, Anime &anime, QStringList &posters)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call("detail", {id}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::Map) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    // {name=xx, <desc=xx>, airdate=xx(yyyy-mm-dd), id=xx, <epcount=xx(int)>, <coverurl=xx>,
    //  <staff=xx("xx:xx;yy:yy;...")>, <crt=[{name=xx,id=xx,<actor=xx>,<link=xx>, <imgurl=xx>},...]>, <posters=[url1, url2,...]>}
    auto aobj = rets[0].toMap();
    QString name = aobj.value("name").toString(), aid=aobj.value("id").toString(), airdate=aobj.value("airdate").toString();
    if(name.isEmpty() || id.isEmpty() || airdate.isEmpty()) return ScriptState(ScriptState::S_ERROR, "Wrong anime info");
    anime.name = name; anime.id = aid; anime.airDate = airdate;
    anime.epCount = aobj.value("epcount", 0).toInt();
    anime.coverURL = aobj.value("coverurl").toString();
    anime.coverPixmap = QPixmap();
    anime.staff.clear();
    if(aobj.contains("staff"))
    {
        QString staffstr = aobj.value("staff").toString();
        auto strs = staffstr.split(';', QString::SkipEmptyParts);
        for(const auto &s : strs)
        {
            int pos = s.indexOf(':');
            anime.staff.append({s.mid(0, pos), s.mid(pos+1)});
        }
    }
    anime.characters.clear();
    if(aobj.contains("crt") && aobj.value("crt").type() == QVariant::List)
    {
        auto crts = aobj.value("crt").toList();
        for(auto &c : crts)
        {
            if(c.type()!=QVariant::Map) continue;
            auto cobj = c.toMap();
            QString cname = cobj.value("name").toString(), cid = cobj.value("id").toString();
            if(cname.isEmpty() || cid.isEmpty()) continue;
            Character crt;
            crt.name = cname; crt.id = cid;
            crt.actor = cobj.value("actor").toString();
            crt.link = cobj.value("link").toString();
            crt.imgURL = cobj.value("imgurl").toString();
            anime.characters.append(crt);
        }
    }
    if(aobj.contains("posters") && aobj.value("posters").type() == QVariant::StringList)
    {
        posters = aobj.value("posters").toStringList();
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::getEp(const QString &id, QList<EpInfo> &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call("getep", {id}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::List) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto eps = rets[0].toList();
    for(auto e: eps)
    {
        auto epobj = e.toMap();
        double index = epobj.value("index", -1).toDouble();
        QString epName = epobj.value("name").toString();
        if(index<0 || epName.isEmpty()) continue;
        EpInfo ep;
        ep.name = epName; ep.index = index;
        ep.type = EpInfo::EpType(qBound(1, epobj.value("type", 1).toInt(), int(EpInfo::EpType::Other)));
        results.append(ep);
    }
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::getTags(const QString &id, QStringList &results)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call("gettags", {id}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::StringList) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    results = rets[0].toStringList();
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::match(const QString &path, MatchResult &result)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    QVariantList rets = call("match", {path}, 1, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    if(rets[0].type()!=QVariant::Map) return ScriptState(ScriptState::S_ERROR, "Wrong Return Value Type");
    auto m = rets[0].toMap(); //{success=xx, anime={name=xx, id=xx}, ep={name=xx, index=number, <type=xx>}}
    do
    {
        if(!m.value("success", false).toBool()) break;
        if(!m.contains("anime") || !m.contains("ep")) break;
        auto anime = m.value("anime"), ep = m.value("ep");
        if(anime.type()!=QVariant::Map || ep.type()!= QVariant::Map) break;
        auto aobj = anime.toMap(), epobj = ep.toMap();
        QString animeName = aobj.value("name").toString(), animeId = aobj.value("id").toString();
        QString epName = epobj.value("name").toString();
        double epIndex = epobj.value("index", -1).toDouble();
        if(animeName.isEmpty() || animeId.isEmpty() || epName.isEmpty() || epIndex<0) break;
        result.success = true;
        AnimeBase *ab = new AnimeBase;
        ab->name = animeName; ab->id = animeId;
        EpInfo *epinfo = new EpInfo;
        epinfo->index = epIndex; epinfo->name = epName;
        epinfo->type = EpInfo::EpType(qBound(1, epobj.value("type", 1).toInt(), int(EpInfo::EpType::Other)));
        result.anime.reset(ab);
        result.ep.reset(epinfo);
        return ScriptState(ScriptState::S_NORM);
    }while(false);
    result.success = false;
    return ScriptState(ScriptState::S_NORM);
}

ScriptState LibraryScript::menuClick(const QString &mid, Anime &anime)
{
    MutexLocker locker(scriptLock);
    if(!locker.tryLock()) return ScriptState(ScriptState::S_BUSY);
    QString errInfo;
    call("menuclick", {mid, anime.toMap()}, 0, errInfo);
    if(!errInfo.isEmpty()) return ScriptState(ScriptState::S_ERROR, errInfo);
    return ScriptState(ScriptState::S_NORM);
}

