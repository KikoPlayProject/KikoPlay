#include "animeinfo.h"
#include "animeworker.h"

Anime::Anime() : crtImagesLoaded(false), epLoaded(false), posterLoaded(false)
{

}

const QList<EpInfo> &Anime::epList()
{
    if(!epLoaded)
    {
        AnimeWorker::instance()->loadEpInfo(this);
        epLoaded = true;
    }
    return epInfoList;
}

const QList<Character> &Anime::crList(bool loadImage)
{
    if(!crtImagesLoaded && loadImage)
    {
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

QVariantMap Anime::toMap()
{
    QVariantList eps;
    for(const auto &ep : epList())
        eps.append(ep.toMap());
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
