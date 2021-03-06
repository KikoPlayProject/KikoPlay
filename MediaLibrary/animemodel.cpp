#include "animemodel.h"
#include "globalobjects.h"
#include "animeworker.h"
#include "Common/notifier.h"
#define AnimeRole Qt::UserRole+1

AnimeModel::AnimeModel(QObject *parent):QAbstractItemModel(parent),
    currentOffset(0),active(false),hasMoreAnimes(false)
{
    limitCount = GlobalObjects::appSetting->value("Library/BatchSize", 128).toInt();
    limitCount = qMax(limitCount, 8);
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::animeAdded, this, &AnimeModel::addAnime);
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::animeRemoved, this, &AnimeModel::removeAnime);
}

void AnimeModel::setActive(bool isActive)
{
    static bool firstActive = true;
    active=isActive;
    if(active)
    {
        if(firstActive)
        {
            firstActive = false;
            fetchMore(QModelIndex());
        }
        if(!tmpAnimes.isEmpty())
        {
            beginInsertRows(QModelIndex(),0,tmpAnimes.count()-1);
            std::copy(tmpAnimes.begin(), tmpAnimes.end(), std::front_inserter(animes));
            tmpAnimes.clear();
            endInsertRows();
            showStatisMessage();
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
    AnimeWorker::instance()->deleteAnime(anime);
    showStatisMessage();
}

Anime *AnimeModel::getAnime(const QModelIndex &index)
{
    if(!index.isValid()) return nullptr;
    Anime *anime=animes.at(index.row());
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
        showStatisMessage();
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
    int totalCount=animes.count();
    if(hasMoreAnimes) totalCount = AnimeWorker::instance()->animeCount();
    emit animeCountInfo(animes.count(), totalCount);
}

QVariant AnimeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const Anime *anime = animes.at(index.row());
    switch (role)
    {
        case Qt::DisplayRole:
        {
            return anime->name();
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
    Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Fetching..."), NM_PROCESS | NM_DARKNESS_BACK);
    AnimeWorker::instance()->fetchAnimes(&moreAnimes, currentOffset, limitCount);
	hasMoreAnimes = moreAnimes.count() >= limitCount;
    if(moreAnimes.count() > 0)
    {
        beginInsertRows(QModelIndex(),animes.count(),animes.count()+moreAnimes.count()-1);
        animes.append(moreAnimes);
        endInsertRows();
        currentOffset+=moreAnimes.count();      
    }
    showStatisMessage();
    Notifier::getNotifier()->showMessage(Notifier::LIBRARY_NOTIFY, tr("Down"), NM_HIDE);
}

