#include "subtitlemodel.h"
#include "globalobjects.h"
#include "subtitleloader.h"
#include "Play/Video/mpvplayer.h"
#include "Common/threadtask.h"
#include "Common/lrucache.h"

namespace
{
    static LRUCache<QString, SubFile> subCache{"Subtitle", 8};
}

SubtitleModel::SubtitleModel(QObject *parent) : QAbstractItemModel{parent}
{
    qRegisterMetaType<SubItem>("SubItem");
    qRegisterMetaType<QList<SubItem>>("QList<SubItem>");

    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::trackInfoChange, this, [=](MPVPlayer::TrackType type){
        if (type == MPVPlayer::TrackType::SubTrack)
        {
            if (blocked)
            {
                if (!curSubFile.items.empty()) reset();
                needRefresh = true;
            }
            else
            {
                refreshSub();
            }
        }
    });
    QObject::connect(GlobalObjects::mpvplayer, &MPVPlayer::stateChanged, this, [this](MPVPlayer::PlayState state) {
        if (state == MPVPlayer::Stop)
        {
            reset();
        }
    });
    refreshSub();
}

QModelIndex SubtitleModel::getTimeIndex(qint64 time)
{
    const QList<SubItem> &curSubItems = curSubFile.items;
    int pos = std::lower_bound(curSubItems.begin(), curSubItems.end(), time, [](const SubItem &item, qint64 time) { return item.startTime < time; }) - curSubItems.begin();
    return pos >= 0 && pos < curSubItems.count() ? createIndex(pos, 0) : QModelIndex();
}

SubItem SubtitleModel::getSub(int row)
{
    const QList<SubItem> &curSubItems = curSubFile.items;
    if (row >= 0 && row < curSubItems.size())
    {
        return curSubItems[row];
    }
    return SubItem();
}

bool SubtitleModel::saveCurSRTSub(const QString &path)
{
    if (curSubFile.format == SubFileFormat::F_SRT)
    {
        QFile subFile(path);
        if (!subFile.open(QIODevice::WriteOnly|QIODevice::Text)) return false;
        subFile.write(curSubFile.rawData.toUtf8());
        return true;
    }
    else
    {
        return curSubFile.saveSRT(path);
    }
}

void SubtitleModel::setBlocked(bool on)
{
    blocked = on;
    if (!blocked)
    {
        if (needRefresh)
        {
            needRefresh = false;
            refreshSub();
        }
    }
}

QVariant SubtitleModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const SubItem &item = curSubFile.items[index.row()];
    Columns col = static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::ToolTipRole:
    {
        switch (col)
        {
        case Columns::TEXT:
        {
            return item.text;
        }
        case Columns::START:
        {
            return item.encodeTime(item.startTime);
        }
        case Columns::END:
        {
            return item.encodeTime(item.endTime);
        }
        }
        break;
    }
    default:
        return QVariant();
    }
    return QVariant();
}

QVariant SubtitleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    static const QStringList headers{ tr("Text"), tr("Start"), tr("End"),  };
    if (role == Qt::DisplayRole && orientation == Qt::Horizontal)
    {
        if (section < headers.size()) return headers.at(section);
    }
    return QVariant();
}

void SubtitleModel::refreshSub()
{
    auto &subTrackList = GlobalObjects::mpvplayer->getTrackList(MPVPlayer::TrackType::SubTrack);
    int trackIdx = GlobalObjects::mpvplayer->getCurrentTrack(MPVPlayer::TrackType::SubTrack);
    if (trackIdx < 0 || trackIdx >= subTrackList.size())
    {
        reset();
        return;
    }

    const MPVPlayer::TrackInfo &track = subTrackList[trackIdx];
    SubFile subFile;
    if (track.isExternal)
    {
        const QString cacheKey = track.externalFile;
        if (subCache.contains(cacheKey))
        {
            subFile = subCache.get(cacheKey);
        }
        else
        {
            SubtitleLoader loader;
            subFile = loader.loadSubFile(track.externalFile);
            subCache.put(cacheKey, subFile);
        }
    }
    else
    {
        const QString cacheKey = QString("%1-%2").arg(GlobalObjects::mpvplayer->getCurrentFile()).arg(track.ffIndex);
        if (subCache.contains(cacheKey))
        {
            subFile = subCache.get(cacheKey);
        }
        else
        {
            ThreadTask task(GlobalObjects::workThread);
            task.Run([&](){
                SubtitleLoader loader;
                SubFileFormat fmt = SubFileFormat::F_SRT;
                if (track.codec.contains("ass")) fmt = SubFileFormat::F_ASS;
                subFile = loader.loadVideoSub(GlobalObjects::mpvplayer->getCurrentFile(), track.ffIndex, fmt);
                return 0;
            });
            subCache.put(cacheKey, subFile);
        }
    }
    beginResetModel();
    this->curSubFile = subFile;
    _hasSub = true;
    endResetModel();
}

void SubtitleModel::reset()
{
    beginResetModel();
    curSubFile = SubFile();
    _hasSub = false;
    endResetModel();
}
