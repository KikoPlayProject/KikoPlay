#include "animeinfo.h"
#include "animeworker.h"

Anime::Anime() : _addTime(0), _epCount(0), crtImagesLoaded(false), epLoaded(false), posterLoaded(false)
{

}

void Anime::assign(const Anime *anime)
{
    _desc = anime->_desc;
    _airDate = anime->_airDate;
    _coverURL = anime->_coverURL;
    _cover = anime->_cover;
    _scriptId = anime->_scriptId;
    _scriptData = anime->_scriptData;
    _epCount = anime->_epCount;
    staff = anime->staff;
    characters = anime->characters;
    crtImagesLoaded = anime->crtImagesLoaded;
}

void Anime::setStaffs(const QString &staffStrs)
{
    staff.clear();
    QStringList staffs(staffStrs.split(';',QString::SkipEmptyParts));
    for(int i=0;i<staffs.count();++i)
    {
        int pos=staffs.at(i).indexOf(':');
        staff.append(QPair<QString,QString>(staffs[i].left(pos),staffs[i].mid(pos+1)));
    }
}

QString Anime::staffToStr() const
{
    QStringList staffStrList;
    for(const auto &p : staff)
        staffStrList.append(p.first+":"+p.second);
    return staffStrList.join(';');
}

const QList<EpInfo> &Anime::epList()
{
    if(!epLoaded)
    {
        epInfoList.clear();
        AnimeWorker::instance()->loadEpInfo(this);
        epLoaded = true;
    }
    return epInfoList;
}

const QList<Character> &Anime::crList(bool loadImage)
{
    if(!crtImagesLoaded && loadImage)
    {
        characters.clear();
        AnimeWorker::instance()->loadCrImages(this);
        crtImagesLoaded = true;
    }
    return characters;
}

const QStringList &Anime::tagList()
{
	return QStringList();
}

const QList<AnimeImage> &Anime::posterList()
{
    if(!posterLoaded)
    {
        posters.clear();
        AnimeWorker::instance()->loadPosters(this);
        posterLoaded = true;
    }
    return posters;
}

void Anime::addEp(const EpInfo &ep)
{
    if(!epLoaded || ep.localFile.isEmpty()) return;
    for(auto &e : epInfoList)
    {
        if(ep.localFile==e.localFile)
        {
            e.index = ep.index;
            e.type = ep.type;
            e.name = ep.name;
            return;
        }
    }
    int pos = 0;
    while(pos<epInfoList.size() && epInfoList[pos]<ep) ++pos;
    epInfoList.insert(pos, ep);
}

void Anime::updateEpTime(const QString &path, qint64 time, bool isFinished)
{
    if(!epLoaded) return;
    for(auto &e : epInfoList)
    {
        if(e.localFile==path)
        {
            if(isFinished) e.finishTime = time;
            else e.lastPlayTime = time;
            return;
        }
    }
}

void Anime::updateEpInfo(const QString &path, const EpInfo &nInfo)
{
    if(!epLoaded) return;
    for(auto &e : epInfoList)
    {
        if(e.localFile==path)
        {
            e.name = nInfo.name;
            e.type = nInfo.type;
            e.index = nInfo.index;
            return;
        }
    }
}

void Anime::updateEpPath(const QString &path, const QString &nPath)
{
    if(!epLoaded) return;
    for(auto &e : epInfoList)
    {
        if(e.localFile==path)
        {
            e.localFile = nPath;
            return;
        }
    }
}

void Anime::removeEp(const QString &epPath)
{
    if(!epLoaded) return;
    for(auto iter=epInfoList.begin(); iter!=epInfoList.end();)
    {
        if(iter->localFile == epPath)
        {
            iter = epInfoList.erase(iter);
        } else {
            ++iter;
        }
    }
}

void Anime::removeEp(EpType type, double index)
{
    if(!epLoaded) return;
    for(auto iter=epInfoList.begin(); iter!=epInfoList.end();)
    {
        if(iter->type == type && iter->index == index)
        {
            iter = epInfoList.erase(iter);
        } else {
            ++iter;
        }
    }
}

void Anime::removePoster(qint64 timeId)
{
    if(!posterLoaded) return;
    for(auto iter=posters.begin(); iter!=posters.end();)
    {
        if(iter->timeId==timeId) iter = posters.erase(iter);
        else ++iter;
    }
}

QVariantMap Anime::toMap(bool fillEp)
{
    QVariantList eps;
    if(fillEp)
    {
        for(const auto &ep : epList())
            eps.append(ep.toMap());
    }
    return
    {
        {"name", _name},
        {"desc", _desc},
        {"url", _url},
        {"scriptData", _scriptData},
        {"airDate", _airDate},
        {"epCount", _epCount},
        {"addTime", QString::number(_addTime)},
        {"eps", eps}
    };
}

const AnimeLite Anime::toLite() const
{
    AnimeLite lite;
    lite.name = _name;
    lite.scriptId = _scriptId;
    lite.scriptData = _scriptData;
    if(epLoaded) lite.epList.reset(new QList<EpInfo>(epInfoList));
    return lite;
}

Anime *AnimeLite::toAnime() const
{
    Anime *anime = new Anime;
    anime->_name = name;
    anime->_scriptId = scriptId;
    anime->_scriptData = scriptData;
    if(epList)
    {
        anime->epInfoList = *epList;
        anime->epLoaded = true;
    }
    return anime;
}

bool operator==(const EpInfo &ep1, const EpInfo &ep2)
{
    return ep1.type==ep2.type && ep1.index==ep2.index;
}

bool operator!=(const EpInfo &ep1, const EpInfo &ep2)
{
    return !(ep1==ep2);
}
