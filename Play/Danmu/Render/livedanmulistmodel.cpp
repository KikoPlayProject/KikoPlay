#include "livedanmulistmodel.h"
#define AlignmentRole Qt::UserRole+1

LiveDanmuListModel::LiveDanmuListModel(QObject *parent)
    : QAbstractItemModel{parent}
{

}

void LiveDanmuListModel::addLiveDanmu(const QVector<DrawTask> &danmuList)
{
    if(danmuList.isEmpty()) return;
    beginInsertRows(QModelIndex(), liveDanmus.size(), liveDanmus.size() + danmuList.size() - 1);
    for(const DrawTask &t : danmuList)
    {
        liveDanmus.push_back(t.comment);
    }
    endInsertRows();
    if(liveDanmus.size() > maxNum)
    {
        int rmSize = liveDanmus.size() - maxNum / 2;
        beginRemoveRows(QModelIndex(), 0, rmSize - 1);
        liveDanmus.erase(liveDanmus.begin(), liveDanmus.begin() + rmSize);
        endRemoveRows();
    }
    emit danmuAppend();
}

void LiveDanmuListModel::clear()
{
    beginResetModel();
    liveDanmus.clear();
    endResetModel();
}

QSharedPointer<DanmuComment> LiveDanmuListModel::getDanmu(const QModelIndex &index)
{
    if(!index.isValid()) return nullptr;
    return liveDanmus.at(index.row());
}

QVariant LiveDanmuListModel::data(const QModelIndex &index, int role) const
{
    if(!index.isValid()) return QVariant();
    const QSharedPointer<DanmuComment> &dm = liveDanmus.at(index.row());
    if(role == Qt::DisplayRole)
    {
        if(dm->mergedList)
        {
            if(mergeCountPos == 0) return dm->text;
            return QString(mergeCountPos == 1? "[%1]%2" : "%2[%1]").arg(dm->mergedList->count()).arg(dm->text);
        }
        return showSender? (dm->sender + ": " + dm->text) : dm->text;
    }
    else if(role == Qt::ForegroundRole)
    {
        return QColor((dm->color>>16)&0xff, (dm->color>>8)&0xff, dm->color&0xff, fontAlpha);
    }
    else if(role == Qt::FontRole)
    {
        if(dm->mergedList && enlargeMerged)
        {
            QFont largerFont(danmuFont);
            largerFont.setPointSize(danmuFont.pointSize() * 1.5);
            return largerFont;
        }
        return danmuFont;
    }
    else if(role == AlignmentRole)
    {
        return align;
    }
    return QVariant();
}
