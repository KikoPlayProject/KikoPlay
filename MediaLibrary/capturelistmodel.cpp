#include "capturelistmodel.h"
#include "globalobjects.h"
#include "animelibrary.h"

CaptureListModel::CaptureListModel(const QString &anime, QObject *parent) : QAbstractListModel(parent),
    animeName(anime),currentOffset(0),hasMoreCaptures(true)
{

}

CaptureListModel::~CaptureListModel()
{
    qDeleteAll(captureList);
}

void CaptureListModel::deleteCaptures(const QModelIndexList &indexes)
{
    QList<int> rows;
    for(const QModelIndex &index : indexes)
    {
        if (index.isValid())
        {
            int row=index.row();
            rows.append(row);
        }
    }
    std::sort(rows.rbegin(),rows.rend());
    for(auto iter=rows.begin();iter!=rows.end();++iter)
    {
        deleteRow(*iter);
    }
}

void CaptureListModel::deleteRow(int row)
{
    if(row<0 || row>=captureList.count()) return;
    CaptureItem *capture=captureList.at(row);
    beginRemoveRows(QModelIndex(), row, row);
    captureList.removeAt(row);
    endRemoveRows();
    GlobalObjects::library->deleteCapture(capture->timeId);
    delete capture;
}

QPixmap CaptureListModel::getFullCapture(int row)
{
    if(row<0 || row>=captureList.count()) return QPixmap();
    return GlobalObjects::library->getCapture(captureList.at(row)->timeId);
}

const CaptureItem *CaptureListModel::getCaptureItem(int row)
{
    if(row<0 || row>=captureList.count()) return nullptr;
    return captureList.at(row);
}

QVariant CaptureListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const CaptureItem *capture = captureList.at(index.row());
    switch (role)
    {
        case Qt::DisplayRole:
        {
            return capture->info;
        }
        case Qt::ToolTipRole:
        {
            return QString("%1\n%2").arg(capture->info,QDateTime::fromSecsSinceEpoch(capture->timeId).toString("yyyy-MM-dd hh:mm:ss"));
        }
        case Qt::DecorationRole:
        {
            QPixmap thumb;
            thumb.loadFromData(capture->thumb);
            return thumb;
        }
    }
    return QVariant();
}

void CaptureListModel::fetchMore(const QModelIndex &)
{
    QList<CaptureItem *> moreCaptures;
    hasMoreCaptures=false;
    emit fetching(true);
    GlobalObjects::library->fetchAnimeCaptures(animeName,moreCaptures,currentOffset,limitCount);
    if(moreCaptures.count()>0)
    {
        hasMoreCaptures=(moreCaptures.count()==limitCount);
        beginInsertRows(QModelIndex(),captureList.count(),captureList.count()+moreCaptures.count()-1);
        captureList.append(moreCaptures);
        endInsertRows();
        currentOffset+=moreCaptures.count();
    }
    emit fetching(false);
}
