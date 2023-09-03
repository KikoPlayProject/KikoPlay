#include "lua_libraryinterface.h"
#include "Extension/Common/ext_common.h"
#include "Common/network.h"
#include "MediaLibrary/animeworker.h"
#include "MediaLibrary/labelmodel.h"
#include <QImageReader>

namespace Extension
{

void LibraryInterface::setup()
{
    const luaL_Reg funcs[] = {
        {"getanime", getanime},
        {"gettag", gettag},
        {"addanime", addanime},
        {"addtag", addtag},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "library"}, funcs);
    addDataMembers({"kiko", "anime"}, {
        { "EP_TYPE_EP", EpType::EP },
        { "EP_TYPE_SP", EpType::SP },
        { "EP_TYPE_OP", EpType::OP },
        { "EP_TYPE_ED", EpType::ED },
        { "EP_TYPE_TRAILER", EpType::Trailer },
        { "EP_TYPE_MAD", EpType::MAD },
        { "EP_TYPE_OTHER", EpType::Other },
    });
}

int LibraryInterface::getanime(lua_State *L)
{
    if (lua_gettop(L) != 1) return 0;
    const QString animeName = lua_tostring(L, 1);
    Anime *anime = AnimeWorker::instance()->getAnime(animeName);
    if(anime)
    {
        pushValue(L, anime->toMap(true));
        return 1;
    }
    return 0;
}

int LibraryInterface::gettag(lua_State *L)
{
    if (lua_gettop(L) != 1) return 0;
    const QString animeName = lua_tostring(L, 1);
    auto &tagMap = LabelModel::instance()->customTags();
    QStringList tags;
    for(auto iter = tagMap.begin(); iter!=tagMap.end(); ++iter)
    {
        if(iter.value().contains(animeName))
        {
            tags<<iter.key();
        }
    }
    pushValue(L, tags);
    return 1;
}

int LibraryInterface::addanime(lua_State *L)
{
    if (lua_gettop(L) != 1) return 0;
    if (lua_type(L, 1) == LUA_TSTRING)
    {
        const QString animeName = QString(lua_tostring(L, 1)).trimmed();
        if (!animeName.isEmpty())
        {
            AnimeWorker::instance()->addAnime(animeName);
            lua_pushboolean(L, true);
            return 1;
        }
        return 0;
    }
    auto aobj = getValue(L, true, 5).toMap();
    Anime *anime = Anime::fromMap(aobj);
    if (!anime) return 0;
    if (!anime->coverURL().isEmpty())
    {
        if(QUrl(anime->coverURL()).isLocalFile())
        {
            QFile coverFile(QUrl(anime->coverURL()).toLocalFile());
            if(coverFile.open(QIODevice::ReadOnly))
            {
                anime->setCover(coverFile.readAll(), false, "");
            }
        }
        else
        {
            Network::Reply reply(Network::httpGet(anime->coverURL(), QUrlQuery()));
            if(!reply.hasError)
            {
                anime->setCover(reply.content, false);
            }
        }
    }
    QStringList urls;
    QList<QUrlQuery> querys;
    QList<Character *> crts;
    for(auto &crt : anime->characters)
    {
        if(QUrl(crt.imgURL).isLocalFile())
        {
            QFile crtImageFile(QUrl(crt.imgURL).toLocalFile());
            if(crtImageFile.open(QIODevice::ReadOnly))
                crt.image.loadFromData(crtImageFile.readAll());
            crt.imgURL = "";
        }
        else
        {
            urls.append(crt.imgURL);
            querys.append(QUrlQuery());
            crts.append(&crt);
        }
    }
    QList<Network::Reply> results(Network::httpGetBatch(urls,querys));
    for(int i = 0; i<crts.size(); ++i)
    {
        if(!results[i].hasError)
        {
            QBuffer bufferImage(&results[i].content);
            bufferImage.open(QIODevice::ReadOnly);
            QImageReader reader(&bufferImage);
            QSize s = reader.size();
            int w = qMin(s.width(), s.height());
            reader.setScaledClipRect(QRect(0, 0, w, w));
            crts[i]->image = QPixmap::fromImageReader(&reader);
            Character::scale(crts[i]->image);
        }
    }
    AnimeWorker::instance()->addAnime(anime);
    return 0;
}

int LibraryInterface::addtag(lua_State *L)
{
    if (lua_gettop(L) != 2 || lua_type(L, 1) != LUA_TSTRING) return 0;
    const QString animeName = QString(lua_tostring(L, 1)).trimmed();
    const QStringList tags = getValue(L, true, 2).toStringList();
    if (animeName.isEmpty() || tags.isEmpty())
    {
        return 0;
    }
    int validCount = 0;
    QMetaObject::invokeMethod(LabelModel::instance(), [&](){
        validCount = LabelModel::instance()->addCustomTags(animeName, tags);
    }, Qt::BlockingQueuedConnection);
    lua_pushinteger(L, validCount);
    return 1;
}


}
