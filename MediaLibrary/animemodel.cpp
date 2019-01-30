#include "animemodel.h"
#include "globalobjects.h"
#include "animelibrary.h"
#define AnimeRole Qt::UserRole+1
AnimeModel::AnimeModel(AnimeLibrary *library):QAbstractItemModel(library),
    currentOffset(0),active(false),hasMoreAnimes(true)
{
    QObject::connect(library,&AnimeLibrary::addAnime,this,&AnimeModel::addAnime);
    QObject::connect(library,&AnimeLibrary::removeOldAnime,this,&AnimeModel::removeAnime);
}

void AnimeModel::setActive(bool isActive)
{
    active=isActive;
    if(active)
    {
        if(!tmpAnimes.isEmpty())
        {
            beginInsertRows(QModelIndex(),0,tmpAnimes.count()-1);
            while(!tmpAnimes.isEmpty())
            {
                Anime *anime=tmpAnimes.takeFirst();
                animes.prepend(anime);
            }
            endInsertRows();
            showStatisMessage();
        }
        else
        {
            static bool firstActive=true;
            if(firstActive)
            {
                showStatisMessage();
                firstActive=false;
            }
        }
    }
}

void AnimeModel::deleteAnime(const QModelIndex &index)
{
    if(!index.isValid())return;
    Anime *anime=animes.at(index.row());
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    animes.removeAt(index.row());
    endRemoveRows();
    currentOffset--;
    GlobalObjects::library->deleteAnime(anime);
    showStatisMessage();
}

Anime *AnimeModel::getAnime(const QModelIndex &index, bool fillInfo)
{
    if(!index.isValid()) return nullptr;
    Anime *anime=animes.at(index.row());
    if(fillInfo) GlobalObjects::library->fillAnimeInfo(anime);
    return anime;
}

void AnimeModel::addAnime(Anime *anime)
{
    if(!active)
    {
        tmpAnimes.append(anime);
    }
    else
    {
        beginInsertRows(QModelIndex(),0,0);
        animes.prepend(anime);
        endInsertRows();
    }
    currentOffset++;
}

void AnimeModel::removeAnime(Anime *anime)
{
    QModelIndex index=createIndex(animes.indexOf(anime),0);
    deleteAnime(index);
}

void AnimeModel::showStatisMessage()
{
    int totalCount=GlobalObjects::library->getTotalAnimeCount();
    if(totalCount==animes.count())
    {
        emit animeMessage(tr("Anime: %1 All Loaded").arg(totalCount),PopMessageFlag::PM_OK,false);
    }
    else
    {
        emit animeMessage(tr("Total Anime: %1  Loaded: %2").arg(totalCount).arg(animes.count()),PopMessageFlag::PM_INFO,true);
    }
}

QVariant AnimeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const Anime *anime = animes.at(index.row());
    switch (role)
    {
        case Qt::DisplayRole:
        {
            return anime->title;
        }
        case AnimeRole:
        {
            return QVariant::fromValue((void *)anime);
        }
    }
    return QVariant();
}

void AnimeModel::fetchMore(const QModelIndex &)
{
    QList<Anime *> moreAnimes;
    hasMoreAnimes=false;
    emit animeMessage(tr("Fetching..."),PopMessageFlag::PM_PROCESS,false);
    GlobalObjects::library->fetchAnimes(moreAnimes,currentOffset,limitCount);
    if(moreAnimes.count()>0)
    {
        hasMoreAnimes=true;
        beginInsertRows(QModelIndex(),animes.count(),animes.count()+moreAnimes.count()-1);
        animes.append(moreAnimes);
        endInsertRows();
        currentOffset+=moreAnimes.count();
    }
    showStatisMessage();
}

