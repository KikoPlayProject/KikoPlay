#include "lua_playlistinterface.h"
#include "Extension/Common/ext_common.h"
#include "globalobjects.h"
#include "Play/Playlist/playlist.h"

namespace Extension
{

void PlayListInterface::setup()
{
    const luaL_Reg funcs[] = {
        {"add", add},
        {"curitem", curitem},
        {"get", get},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "playlist"}, funcs);
    addDataMembers({"kiko", "playlist"}, {
        { "ITEM_LOCAL_FILE", PlayListItem::ItemType::LOCAL_FILE },
        { "ITEM_WEB_URL", PlayListItem::ItemType::WEB_URL },
        { "ITEM_COLLECTION", PlayListItem::ItemType::COLLECTION },
    });
}

int PlayListInterface::add(lua_State *L)
{
    if (lua_gettop(L) != 1)
    {
        lua_pushboolean(L, false);
        return 1;
    }
    QVariantMap itemInfo = getValue(L, true, 2).toMap();
    do
    {
        if (!itemInfo.contains("title")) break;
        QString title = itemInfo["title"].toString();
        if (title.isEmpty()) break;
        const int type = itemInfo.value("src_type", -1).toInt();
        if (type == PlayListItem::ItemType::LOCAL_FILE || type == PlayListItem::ItemType::WEB_URL)
        {
            if (!itemInfo.contains("path")) break;
            QString path = itemInfo["path"].toString();
            if (path.isEmpty()) break;
            QStringList insertPosition;
            if (itemInfo.contains("position")) insertPosition = itemInfo["position"].toString().split("/", Qt::SkipEmptyParts);
            PlayListItem *item = new PlayListItem(nullptr, true);
            item->title = title;
            item->path = type == PlayListItem::ItemType::LOCAL_FILE ? QFileInfo(path).absoluteFilePath() : path;
            item->type = static_cast<PlayListItem::ItemType>(type);
            item->animeTitle = itemInfo.value("anime_title").toString();
            item->poolID = itemInfo.value("pool").toString();
            QMetaObject::invokeMethod(GlobalObjects::playlist, [&](){
                QModelIndex parent;
                if (!insertPosition.empty())
                {
                    parent = GlobalObjects::playlist->getCollection(parent, insertPosition);
                }
                GlobalObjects::playlist->addItem(parent, item);
            }, Qt::BlockingQueuedConnection);
            lua_pushboolean(L, true);
            return 1;
        }
        else
        {
            QStringList insertPosition;
            if (itemInfo.contains("position")) insertPosition = itemInfo["position"].toString().split("/", Qt::SkipEmptyParts);
            QString path = itemInfo.value("path").toString();
            bool isBgmCollection = itemInfo.value("bgm_collection").toBool();
            bool isWebDAVCollection = itemInfo.value("webdav_collection").toBool();
            const QString webDAVUser = itemInfo.value("webdav_user").toString();
            const QString webDAVPassword = itemInfo.value("webdav_password").toString();
            QMetaObject::invokeMethod(GlobalObjects::playlist, [&](){
                if (!path.isEmpty())
                {
                    QModelIndex parent;
                    if (!insertPosition.empty())
                    {
                        parent = GlobalObjects::playlist->getCollection(parent, insertPosition);
                    }
                    if (isWebDAVCollection)
                    {
                        GlobalObjects::playlist->addWebDAVCollection(parent, title, path, webDAVUser, webDAVPassword);
                    }
                    else
                    {
                        GlobalObjects::playlist->addFolder(path, parent, title);
                    }
                }
                else
                {
                    insertPosition.append(title);
                    QModelIndex itemIndex = GlobalObjects::playlist->getCollection(QModelIndex(), insertPosition);
                    if (isBgmCollection)
                    {
                        GlobalObjects::playlist->switchBgmCollection(itemIndex);
                    }
                }
            }, Qt::BlockingQueuedConnection);
            lua_pushboolean(L, true);
            return 1;
        }
    }
    while (false);
    lua_pushboolean(L, false);
    return 1;
}

int PlayListInterface::curitem(lua_State *L)
{
    QVariant itemInfo;
    QMetaObject::invokeMethod(GlobalObjects::playlist, [&](){
        const PlayListItem *item = GlobalObjects::playlist->getCurrentItem();
        if (!item) return;
        QStringList pathStack;
        const PlayListItem *tmpItem = item->parent;
        while(tmpItem)
        {
            pathStack.push_front(tmpItem->title);
            tmpItem = tmpItem->parent;
        }
        QVariantMap info = getItemInfo(item);
        info["position"] = pathStack.join('/');
        itemInfo = info;
    }, Qt::BlockingQueuedConnection);
    if (itemInfo.isNull()) lua_pushnil(L);
    else pushValue(L, itemInfo);
    return 1;
}

int PlayListInterface::get(lua_State *L)
{
    if (lua_gettop(L) < 1 || lua_type(L, 1) != LUA_TSTRING)
    {
        return 0;
    }
    const QStringList path = QString(lua_tostring(L, 1)).split("/", Qt::SkipEmptyParts);
    QVariant itemInfo;
    QMetaObject::invokeMethod(GlobalObjects::playlist, [&](){
        const PlayListItem *item = GlobalObjects::playlist->getItem(QModelIndex(), path);
        if (!item) return;
        QStringList pathStack;
        const PlayListItem *tmpItem = item->parent;
        while(tmpItem)
        {
            pathStack.push_front(tmpItem->title);
            tmpItem = tmpItem->parent;
        }
        QVariantMap info = getItemInfo(item);
        info["position"] = pathStack.join('/');
        if (item->children && !item->children->empty())
        {
            pathStack.push_back(item->title);
            const QString childPos = pathStack.join('/');
            QVariantList childrenInfo;
            for (const PlayListItem *c : *item->children)
            {
                QVariantMap i = getItemInfo(c);
                i["position"] = childPos;
                childrenInfo.append(i);
            }
            info["children"] = childrenInfo;
        }
        itemInfo = info;
    }, Qt::BlockingQueuedConnection);
    if (itemInfo.isNull()) lua_pushnil(L);
    else pushValue(L, itemInfo);
    return 1;
}

QVariantMap PlayListInterface::getItemInfo(const PlayListItem *item)
{
    if (!item) return QVariantMap();
    return QVariantMap({
       {"src_type", item->type},
       {"state", item->playTimeState},
       {"bgm_collection", item->isBgmCollection},
       {"webdav_collection", item->isWebDAVCollection()},
       {"add_time", item->addTime},
       {"title", item->title},
       {"anime_title", item->animeTitle},
       {"path", item->path},
       {"pool", item->poolID},
   });
}


}
