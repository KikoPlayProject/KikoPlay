#include "episodesmodel.h"
#include "globalobjects.h"
#include "animeworker.h"
EpisodesModel::EpisodesModel(Anime *anime, QObject *parent) : QAbstractItemModel(parent),
    currentAnime(nullptr)
{
    setAnime(anime);
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::epUpdated, this, [=](const QString &animeName, const QString &epPath){
       if(!currentAnime || currentAnime->name()!=animeName) return;
       int pos = epMap[epPath];
       EpInfo &ep = currentEps[pos];
       ep = currentAnime->epList()[pos];
       if(ep.localFile!=epPath)
       {
           epMap.remove(epPath);
           epMap[ep.localFile] = pos;
       }
       QModelIndex modelIndex(this->index(pos, (int)Columns::TITLE, QModelIndex()));
       emit dataChanged(modelIndex, modelIndex.siblingAtColumn((int)Columns::LOCALFILE));
    });
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::epAdded, this, [=](const QString &animeName, const EpInfo &ep){
         if(!currentAnime || currentAnime->name()!=animeName || epMap.contains(ep.localFile)) return;
         int pos = currentEps.size();
         beginInsertRows(QModelIndex(), pos, pos);
         currentEps.append(ep);
         epMap[ep.localFile] = pos;
         endInsertRows();
    });
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::epRemoved, this, [=](const QString &animeName, const QString &epPath){
         if(!currentAnime || currentAnime->name()!=animeName || !epMap.contains(epPath)) return;
         int pos = epMap[epPath];
         epMap.remove(epPath);
         for(int i=pos+1;i<currentEps.size();++i)
         {
             epMap[currentEps[i].localFile] = i-1;
         }
         beginRemoveRows(QModelIndex(), pos, pos);
         currentEps.removeAt(pos);
         endRemoveRows();
    });
}

void EpisodesModel::setAnime(Anime *anime)
{
    beginResetModel();
    currentAnime = anime;
    currentEps.clear();
    epMap.clear();
    if(anime)
    {
        currentEps = anime->epList();
        for(int i=0; i<currentEps.size(); ++i)
        {
            epMap[currentEps[i].localFile] = i;
        }
    }
    endResetModel();
}

void EpisodesModel::addEp(const QString &path)
{
    if(!currentAnime || epMap.contains(path)) return;
    EpInfo ep;
    ep.localFile = path;
    ep.type = EpType::EP;
    ep.name = QFileInfo(path).baseName();
    AnimeWorker::instance()->addEp(currentAnime->name(), ep);
}

void EpisodesModel::removeEp(const QModelIndex &index)
{
    if(!currentAnime || !index.isValid()) return;
    EpInfo &ep = currentEps[index.row()];
    AnimeWorker::instance()->removeEp(currentAnime->name(), ep.localFile);
}

int EpisodesModel::removeInvalidEp()
{
    if(!currentAnime) return 0;
    int invalidCount = 0;
    for(int i=currentEps.size()-1;i>=0;--i)
    {
        QString epPath(currentEps[i].localFile);
        QFileInfo info(epPath);
        if(!info.exists())
        {
            ++invalidCount;
            epMap.remove(epPath);
            beginRemoveRows(QModelIndex(), i, i);
            currentEps.removeAt(i);
            endRemoveRows();
            AnimeWorker::instance()->removeEp(currentAnime->name(), epPath);
        }
    }
    if(invalidCount>0)
    {
        epMap.clear();
        for(int i=0; i<currentEps.size(); ++i)
        {
            epMap[currentEps[i].localFile] = i;
        }
    }
    return invalidCount;
}

void EpisodesModel::updateEpInfo(const QModelIndex &index, const EpInfo &nEpInfo)
{
    if(!currentAnime || !index.isValid()) return;
    AnimeWorker::instance()->updateEpInfo(currentAnime->name(), currentEps[index.row()].localFile, nEpInfo);
}

void EpisodesModel::updateFinishTime(const QModelIndex &index, qint64 nTime)
{
    if(!currentAnime || !index.isValid()) return;
    AnimeWorker::instance()->updateEpTime(currentAnime->name(), currentEps[index.row()].localFile, true, nTime);
}

QVariant EpisodesModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid() || !currentAnime) return QVariant();
    const auto &ep=currentEps.at(index.row());
    Columns col=static_cast<Columns>(index.column());
    if(role==Qt::DisplayRole)
    {
        switch (col)
        {
        case Columns::TITLE:
            return ep.toString();
        case Columns::LASTPLAY:
            return ep.playTimeStr();
        case Columns::FINISHTIME:
            return ep.playTimeStr(true);
        case Columns::LOCALFILE:
            return ep.localFile;
        }
    }
    else if(role == EpRole)
    {
        return QVariant::fromValue(ep);
    }
    return QVariant();
}

bool EpisodesModel::setData(const QModelIndex &index, const QVariant &value, int )
{
    if(!index.isValid() || !currentAnime) return false;
    Columns col=static_cast<Columns>(index.column());
    auto &ep=currentEps[index.row()];
    switch (col)
    {
    case Columns::TITLE:
    {
        EpInfo nEp(value.value<EpInfo>());
        if(ep.name==nEp.name && ep.type==nEp.type && ep.index==nEp.index)return false;
        AnimeWorker::instance()->updateEpInfo(currentAnime->name(),ep.localFile, nEp);
        break;
    }
    case Columns::FINISHTIME:
    {
        qint64 nTime = value.toLongLong();
        if(nTime==ep.finishTime) return false;
        AnimeWorker::instance()->updateEpTime(currentAnime->name(),ep.localFile,true,nTime);
        break;
    }
    case Columns::LOCALFILE:
    {
        QString nPath = value.toString();
        if(nPath==ep.localFile || nPath.isEmpty()) return false;
        AnimeWorker::instance()->updateEpPath(currentAnime->name(), ep.localFile, nPath);
        break;
    }
    default:
        return false;
    }
    return true;
}

QVariant EpisodesModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size())return headers.at(section);
    }
    return QVariant();
}

Qt::ItemFlags EpisodesModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if(!index.isValid()) return defaultFlags;
    Columns col = static_cast<Columns>(index.column());
    if(col==Columns::LASTPLAY) return defaultFlags;
    return defaultFlags | Qt::ItemIsEditable;
}
