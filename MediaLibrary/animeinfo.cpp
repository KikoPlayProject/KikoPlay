#include "animeinfo.h"
#include "animeworker.h"

Anime::Anime() : crtImagesLoaded(false), epLoaded(false), posterLoaded(false)
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
    if(!epLoaded) return;
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
    epInfoList.append(ep);
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
        {"scriptData", _scriptData},
        {"airDate", _airDate},
        {"epCount", _epCount},
        {"addTime", QString::number(_addTime)},
        {"eps", eps}
    };
}
