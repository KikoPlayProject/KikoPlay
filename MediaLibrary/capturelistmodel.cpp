#include "capturelistmodel.h"
#include "globalobjects.h"
#include "animeworker.h"

CaptureListModel::CaptureListModel(const QString &animeName, QObject *parent) : QAbstractListModel(parent), currentOffset(0),hasMoreCaptures(true)
{
    this->animeName = animeName;
    if(animeName.isEmpty()) hasMoreCaptures = false;
    QObject::connect(AnimeWorker::instance(), &AnimeWorker::captureUpdated, this, [=](const QString &animeName, const AnimeImage &aImage){
        if(animeName==this->animeName)
        {
            beginInsertRows(QModelIndex(),0,0);
            captureList.prepend(aImage);
            endInsertRows();
            ++currentOffset;
        }
    });
}

void CaptureListModel::deleteRow(int row)
{
    if(row<0 || row>=captureList.count()) return;
    const AnimeImage &img = captureList.at(row);
    AnimeWorker::instance()->deleteAnimeImage(animeName, img.type, img.timeId);
    QString snippetFilePath(getSnippetFile(row));
    if(!snippetFilePath.isEmpty())
    {
        QFile::remove(snippetFilePath);
    }
    beginRemoveRows(QModelIndex(), row, row);
    captureList.removeAt(row);
    endRemoveRows();
}

QPixmap CaptureListModel::getFullCapture(int row)
{
    if(row<0 || row>=captureList.count()) return QPixmap();
    const AnimeImage &img = captureList.at(row);
    return AnimeWorker::instance()->getAnimeImageData(animeName, img.type, img.timeId);
}

const AnimeImage *CaptureListModel::getCaptureItem(int row)
{
    if(row<0 || row>=captureList.count()) return nullptr;
    return &captureList.at(row);
}

void CaptureListModel::setAnimeName(const QString &name)
{
    beginResetModel();
    captureList.clear();
    animeName = name;
    hasMoreCaptures = !name.isEmpty();
    currentOffset = 0;
    endResetModel();
}

const QString CaptureListModel::getSnippetFile(int row)
{
    if (row<0 || row>=captureList.count()) return "";
    const AnimeImage &img = captureList.at(row);
    if (img.type != AnimeImage::SNIPPET) return "";
    QString snippetPath(GlobalObjects::appSetting->value("Play/SnippetPath", GlobalObjects::context()->dataPath + "/snippet").toString());
    QDir dir(snippetPath, QString("%1.*").arg(img.timeId));
    const QFileInfoList list = dir.entryInfoList();
    if (list.empty()) return "";
    return list.first().absoluteFilePath();
}

QVariant CaptureListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const AnimeImage &capture = captureList.at(index.row());
    switch (role)
    {
        case Qt::DisplayRole:
        case Qt::EditRole:
        {
            return capture.info;
        }
        case Qt::ToolTipRole:
        {
            return QString("%1\n%2").arg(capture.info,QDateTime::fromMSecsSinceEpoch(capture.timeId).toString("yyyy-MM-dd hh:mm:ss"));
        }
        case Qt::DecorationRole:
        {
            return capture.thumb;
        }
    }
    return QVariant();
}

void CaptureListModel::fetchMore(const QModelIndex &)
{
    QList<AnimeImage> moreCaptures;
    hasMoreCaptures=false;
    emit fetching(true);
    AnimeWorker::instance()->fetchCaptures(animeName, moreCaptures, currentOffset, limitCount);
    if (moreCaptures.count() > 0)
    {
        hasMoreCaptures=(moreCaptures.count()==limitCount);
        beginInsertRows(QModelIndex(),captureList.count(),captureList.count()+moreCaptures.count()-1);
        captureList.append(moreCaptures);
        endInsertRows();
        currentOffset+=moreCaptures.count();
    }
    emit fetching(false);
}

Qt::ItemFlags CaptureListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);
    if (index.isValid() && index.column()==0)
    {
        return  Qt::ItemIsEditable | defaultFlags;
    }
    return defaultFlags;
}

bool CaptureListModel::setData(const QModelIndex &index, const QVariant &value, int )
{
    if(!index.isValid()) return false;
    AnimeImage &capture = captureList[index.row()];
    if(index.column()==0)
    {
        capture.info = value.toString();
        AnimeWorker::instance()->updateCaptureInfo(animeName, capture.timeId, capture.info);
        return true;
    }
    return false;
}
