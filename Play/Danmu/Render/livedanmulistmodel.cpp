#include "livedanmulistmodel.h"
#include "globalobjects.h"
#define AlignmentRole Qt::UserRole+1
#define PrefixLengthRole Qt::UserRole+2
#define SuffixLengthRole Qt::UserRole+3
#define AlphaRole Qt::UserRole+4

#define SETTING_KEY_LIVE_ALIGN_RIGHT "Play/LiveModeAlignRight"
#define SETTING_KEY_LIVE_SHOW_SENDER "Play/LiveShowSender"
#define SETTING_KEY_LIVE_DANMU_FONT_SIZE "Play/LiveDanmuFontSize"


LiveDanmuListModel::LiveDanmuListModel(QObject *parent)
    : QAbstractItemModel{parent}
{
    align = GlobalObjects::appSetting->value(SETTING_KEY_LIVE_ALIGN_RIGHT, false).toBool()? Qt::AlignRight : Qt::AlignLeft;
    showSender = GlobalObjects::appSetting->value(SETTING_KEY_LIVE_SHOW_SENDER, true).toBool();
    danmuFont.setPointSize(GlobalObjects::appSetting->value(SETTING_KEY_LIVE_DANMU_FONT_SIZE, 10).toInt());
}

void LiveDanmuListModel::addLiveDanmu(const QVector<DrawTask> &danmuList)
{
    if (danmuList.isEmpty()) return;
    beginInsertRows(QModelIndex(), liveDanmus.size(), liveDanmus.size() + danmuList.size() - 1);
    for (const DrawTask &t : danmuList)
    {
        int r = (t.comment->color>>16) & 0xff, g = (t.comment->color >> 8) & 0xff, b = t.comment->color & 0xff;
        if (isRandomColor)
        {
            r = QRandomGenerator::global()->bounded(0, 256);
            g = QRandomGenerator::global()->bounded(0, 256);
            b = QRandomGenerator::global()->bounded(0, 256);
        }
        liveDanmus.push_back({t.comment, QColor(r, g, b)});
    }
    endInsertRows();
    if (liveDanmus.size() > maxNum)
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
    if (!index.isValid()) return nullptr;
    return liveDanmus.at(index.row()).first;
}

void LiveDanmuListModel::setShowSender(bool show)
{
    showSender = show;
    GlobalObjects::appSetting->setValue(SETTING_KEY_LIVE_SHOW_SENDER, show);
}

void LiveDanmuListModel::setFontSize(int size)
{
    danmuFont.setPointSize(size);
    GlobalObjects::appSetting->setValue(SETTING_KEY_LIVE_DANMU_FONT_SIZE, size);
}

void LiveDanmuListModel::setAlignment(Qt::Alignment alignment)
{
    align = alignment;
    GlobalObjects::appSetting->setValue(SETTING_KEY_LIVE_ALIGN_RIGHT, alignment == Qt::AlignRight);
}

QVariant LiveDanmuListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const QSharedPointer<DanmuComment> &dm = liveDanmus.at(index.row()).first;
    if (role == Qt::DisplayRole)
    {
        if (dm->mergedList)
        {
            if(mergeCountPos == 0) return dm->text;
            return QString(mergeCountPos == 1? "[%1]%2" : "%2[%1]").arg(dm->mergedList->count()).arg(dm->text);
        }
        return showSender? (dm->sender + ": " + dm->text) : dm->text;
    }
    else if (role == Qt::ForegroundRole)
    {
        QColor c = liveDanmus.at(index.row()).second;
        c.setAlpha(fontAlpha);
        return c;
    }
    else if (role == Qt::FontRole)
    {
        if (dm->mergedList && enlargeMerged)
        {
            QFont largerFont(danmuFont);
            largerFont.setPointSize(danmuFont.pointSize() * 1.5);
            return largerFont;
        }
        return danmuFont;
    }
    else if (role == AlignmentRole)
    {
        return align;
    }
    else if (role == AlphaRole)
    {
        return fontAlpha;
    }
    else if (role == PrefixLengthRole)
    {
        if (dm->mergedList)
        {
            if (mergeCountPos == 1) return QString("[%1]").arg(dm->mergedList->count()).length();
        }
        else
        {
            if (showSender) return dm->sender.length() + 1;
        }
        return 0;
    }
    else if (role == SuffixLengthRole)
    {
        if (dm->mergedList && mergeCountPos == 2)
        {
            return QString("[%1]").arg(dm->mergedList->count()).length();
        }
        return 0;
    }
    return QVariant();
}
