#include "playcontext.h"
#include "Play/Playlist/playlistitem.h"
#include "Play/Playlist/playlist.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Danmu/Render/danmurender.h"
#include "globalobjects.h"

PlayContext::PlayContext(QObject *parent)
    : QObject{parent}, duration(0), playtime(0), seekable(false), curItem(nullptr)
{

}

PlayContext *PlayContext::context()
{
    static PlayContext ctx;
    return &ctx;
}

void PlayContext::playItem(const PlayListItem *item)
{
    if (!item) return;
    clear();
    curItem = item;
    path = item->path;
    GlobalObjects::danmuRender->cleanup();
    GlobalObjects::mpvplayer->setMedia(path);
}

void PlayContext::playURL(const QString &url)
{
    const PlayListItem *item = GlobalObjects::playlist->setCurrentItem(url);
    if (item)
    {
        playItem(item);
        return;
    }
    GlobalObjects::playlist->cleanCurrentItem();
    clear();
    curItem = nullptr;
    path = url;
    GlobalObjects::mpvplayer->setMedia(path);
}

void PlayContext::clear()
{
    curItem = nullptr;
    path = "";
    duration = 0;
    playtime = 0;
    seekable = false;
}
