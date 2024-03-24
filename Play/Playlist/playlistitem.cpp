#include "playlistitem.h"
#include "playlist.h"
#include "Play/Danmu/Manager/danmumanager.h"
#include "globalobjects.h"

#define XML_FIELD_TITLE "title"
#define XML_FIELD_ANIME_TITLE "animeTitle"
#define XML_FIELD_POOLID "poolID"
#define XML_FIELD_TYPE "type"
#define XML_FIELD_PLAYTIME "playTime"
#define XML_FIELD_PLAYTIME_STATE "playTimeState"
#define XML_FIELD_TRACK_SUB_DELAY "subDelay"
#define XML_FIELD_TRACK_AUDIO_INDEX "audioIndex"
#define XML_FIELD_TRACK_SUB_INDEX "subIndex"
#define XML_FIELD_TRACK_SUB "sub"
#define XML_FIELD_TRACK_AUDIO "audio"
#define XML_FIELD_BGM_COLLECTION "bgmCollection"
#define XML_FIELD_FOLDER_PATH "folderPath"
#define XML_FIELD_MARKER "marker"
#define XML_FIELD_ADD_TIME "addTime"
#define XML_FIELD_WEBDAV_USER "webDAVUser"
#define XML_FIELD_WEBDAV_PASSWORD "webDAVPassword"

PlayList* PlayListItem::playlist=nullptr;

PlayListItem::PlayListItem(PlayListItem *p, bool leaf, int insertPosition):
    parent(p), children(nullptr), type(LOCAL_FILE), playTimeState(UNPLAY), marker(M_NONE), playTime(0), level(0), isBgmCollection(false), addTime(0),
    trackInfo(nullptr), webDAVInfo(nullptr)
{
    if (!leaf)
    {
        children = new QVector<PlayListItem *>();
        type = ItemType::COLLECTION;
    }
    if (parent)
    {
        if (insertPosition == -1)
            parent->children->append(this);
        else
            parent->children->insert(insertPosition, this);
        level = parent->level + 1;
    }
}
PlayListItem::~PlayListItem()
{
    playlist->checkCurrentItem(this);
    if (children)
    {
        qDeleteAll(children->begin(),children->end());
        delete children;
    }
    if (trackInfo)
    {
        delete trackInfo;
    }
    if (webDAVInfo)
    {
        delete webDAVInfo;
    }
}

PlayListItem *PlayListItem::parseCollection(QXmlStreamReader &reader, PlayListItem *parent)
{
    QXmlStreamAttributes attrs = reader.attributes();
    PlayListItem *collection = new PlayListItem(parent, false);
    collection->title = attrs.value(XML_FIELD_TITLE).toString();
    collection->isBgmCollection = (attrs.value(XML_FIELD_BGM_COLLECTION) == "true");
    collection->path = attrs.value(XML_FIELD_FOLDER_PATH).toString();
    if (attrs.hasAttribute(XML_FIELD_MARKER))
        collection->marker = PlayListItem::Marker(attrs.value(XML_FIELD_MARKER).toInt());
    if (attrs.hasAttribute(XML_FIELD_ADD_TIME))
        collection->addTime = attrs.value(XML_FIELD_ADD_TIME).toLongLong();
    bool hasWebDAVInfo = attrs.hasAttribute(XML_FIELD_WEBDAV_USER) || attrs.hasAttribute(XML_FIELD_WEBDAV_PASSWORD);
    if (hasWebDAVInfo)
    {
        collection->webDAVInfo = new WebDAVInfo;
        collection->webDAVInfo->user = attrs.value(XML_FIELD_WEBDAV_USER).toString();
        collection->webDAVInfo->password = attrs.value(XML_FIELD_WEBDAV_PASSWORD).toString();
    }
    return collection;
}

void PlayListItem::writeCollection(QXmlStreamWriter &writer, PlayListItem *item)
{
    if (!item->isCollection()) return;
    writer.writeAttribute(XML_FIELD_TITLE, item->title);
    writer.writeAttribute(XML_FIELD_BGM_COLLECTION,item->isBgmCollection ? "true" : "false");
    if (!item->path.isEmpty())
        writer.writeAttribute(XML_FIELD_FOLDER_PATH, item->path);
    if (item->marker != PlayListItem::Marker::M_NONE)
        writer.writeAttribute(XML_FIELD_MARKER, QString::number((int)item->marker));
    if (item->addTime > 0)
        writer.writeAttribute(XML_FIELD_ADD_TIME, QString::number(item->addTime));
    if (item->isWebDAVCollection())
    {
        writer.writeAttribute(XML_FIELD_WEBDAV_USER, item->webDAVInfo->user);
        writer.writeAttribute(XML_FIELD_WEBDAV_PASSWORD, item->webDAVInfo->password);
    }
}

PlayListItem *PlayListItem::parseItem(QXmlStreamReader &reader, PlayListItem *parent)
{
    QXmlStreamAttributes attrs = reader.attributes();
    const QString title = attrs.value(XML_FIELD_TITLE).toString();
    const QString animeTitle = attrs.value(XML_FIELD_ANIME_TITLE).toString();
    const QString poolID = attrs.value(XML_FIELD_POOLID).toString();
    PlayListItem::ItemType type = PlayListItem::ItemType::LOCAL_FILE;
    if (attrs.hasAttribute(XML_FIELD_TYPE))
        type = static_cast<PlayListItem::ItemType>(attrs.value(XML_FIELD_TYPE).toInt());
    const int playTime = attrs.value(XML_FIELD_PLAYTIME).toInt();
    const int playTimeState = attrs.value(XML_FIELD_PLAYTIME_STATE).toInt();
    int marker = PlayListItem::Marker::M_NONE;
    if (attrs.hasAttribute(XML_FIELD_MARKER))
        marker = attrs.value(XML_FIELD_MARKER).toInt();
    qint64 addTime = 0;
    if (attrs.hasAttribute(XML_FIELD_ADD_TIME))
        addTime = attrs.value(XML_FIELD_ADD_TIME).toLongLong();
    bool hasTrackInfo = attrs.hasAttribute(XML_FIELD_TRACK_SUB_DELAY) ||
            attrs.hasAttribute(XML_FIELD_TRACK_AUDIO_INDEX) ||
            attrs.hasAttribute(XML_FIELD_TRACK_SUB_INDEX) ||
            attrs.hasAttribute(XML_FIELD_TRACK_SUB) || attrs.hasAttribute(XML_FIELD_TRACK_AUDIO);
    ItemTrackInfo trackInfo;
    if (hasTrackInfo)
    {
        if(attrs.hasAttribute(XML_FIELD_TRACK_SUB_DELAY))
            trackInfo.subDelay = attrs.value(XML_FIELD_TRACK_SUB_DELAY).toInt();
        if(attrs.hasAttribute(XML_FIELD_TRACK_AUDIO_INDEX))
            trackInfo.audioIndex = attrs.value(XML_FIELD_TRACK_AUDIO_INDEX).toInt();
        if(attrs.hasAttribute(XML_FIELD_TRACK_SUB_INDEX))
            trackInfo.subIndex = attrs.value(XML_FIELD_TRACK_SUB_INDEX).toInt();
        if(attrs.hasAttribute(XML_FIELD_TRACK_SUB))
            trackInfo.subFiles = attrs.value(XML_FIELD_TRACK_SUB).toString().split('|', Qt::SkipEmptyParts);
        if(attrs.hasAttribute(XML_FIELD_TRACK_AUDIO))
            trackInfo.audioFiles = attrs.value(XML_FIELD_TRACK_AUDIO).toString().split('|', Qt::SkipEmptyParts);
    }

    const QString path = reader.readElementText().trimmed();

    PlayListItem *item = new PlayListItem(parent, true);
    item->title = title;
    item->path = path;
    item->type = type;
    item->pathHash = QCryptographicHash::hash(path.toUtf8(),QCryptographicHash::Md5).toHex();
    item->playTime = playTime;
    item->poolID = poolID;
    item->playTimeState = PlayListItem::PlayState(playTimeState);
    item->marker = PlayListItem::Marker(marker);
    item->addTime = addTime;
    if (hasTrackInfo) item->trackInfo = new ItemTrackInfo(trackInfo);
    if (!animeTitle.isEmpty()) item->animeTitle = animeTitle;
    return item;
}

void PlayListItem::writeItem(QXmlStreamWriter &writer, PlayListItem *item)
{
    if (item->isCollection()) return;
    writer.writeAttribute(XML_FIELD_TITLE, item->title);
    if (!item->animeTitle.isEmpty())
        writer.writeAttribute(XML_FIELD_ANIME_TITLE, item->animeTitle);
    if (!item->poolID.isEmpty())
        writer.writeAttribute(XML_FIELD_POOLID, item->poolID);
    if (item->type != PlayListItem::ItemType::LOCAL_FILE)
        writer.writeAttribute(XML_FIELD_TYPE, QString::number(item->type));
    writer.writeAttribute(XML_FIELD_PLAYTIME, QString::number(item->playTime));
    writer.writeAttribute(XML_FIELD_PLAYTIME_STATE, QString::number((int)item->playTimeState));
    if (item->marker != PlayListItem::Marker::M_NONE)
        writer.writeAttribute(XML_FIELD_MARKER, QString::number((int)item->marker));
    if (item->addTime > 0)
        writer.writeAttribute(XML_FIELD_ADD_TIME, QString::number(item->addTime));
    if (item->trackInfo)
    {
        if(item->trackInfo->subDelay != 0)
            writer.writeAttribute(XML_FIELD_TRACK_SUB_DELAY, QString::number(item->trackInfo->subDelay));
        if(item->trackInfo->audioIndex > -1)
            writer.writeAttribute(XML_FIELD_TRACK_AUDIO_INDEX, QString::number(item->trackInfo->audioIndex));
        if(item->trackInfo->subIndex > -1)
            writer.writeAttribute(XML_FIELD_TRACK_SUB_INDEX, QString::number(item->trackInfo->subIndex));
        if(!item->trackInfo->subFiles.isEmpty())
            writer.writeAttribute(XML_FIELD_TRACK_SUB, item->trackInfo->subFiles.join('|'));
        if(!item->trackInfo->audioFiles.isEmpty())
            writer.writeAttribute(XML_FIELD_TRACK_AUDIO, item->trackInfo->audioFiles.join('|'));
    }
    writer.writeCharacters(item->path);
}

bool PlayListItem::hasPool() const
{
    return !poolID.isEmpty() && GlobalObjects::danmuManager->getPool(poolID, false);
}
void PlayListItem::setLevel(int newLevel)
{
    level = newLevel;
    if (children)
    {
        for (PlayListItem *child : qAsConst(*children))
        {
            child->setLevel(newLevel+1);
        }
    }
}
void PlayListItem::moveTo(PlayListItem *newParent, int insertPosition)
{
    if (parent) parent->children->removeAll(this);
    if (newParent)
    {
        if (insertPosition == -1)
            newParent->children->append(this);
        else
            newParent->children->insert(insertPosition, this);
        setLevel(newParent->level + 1);
    }
    parent = newParent;
}
