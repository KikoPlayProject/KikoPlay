#include "lua_downloadinterface.h"
#include "globalobjects.h"
#include "Download/downloadmodel.h"
#include "Download/torrent.h"
#include "Extension/App/kapp.h"
#include "UI/adduritask.h"
#include "UI/selecttorrentfile.h"

namespace  Extension
{

void DownloadInterface::setup()
{
    const luaL_Reg funcs[] = {
        {"addurl", addurl},
        {"addtorrent", addtorrent},
        {nullptr, nullptr}
    };
    registerFuncs({"kiko", "download"}, funcs);
}

int DownloadInterface::addurl(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    const int params = lua_gettop(L);
    if (!app || !app->window() || params < 1) return 0;
    auto urlInfo = getValue(L, true, 3).toMap();
    QStringList urls;
    if (urlInfo.value("url").canConvert(QMetaType::QStringList))
    {
        urls = urlInfo.value("url").toStringList();
    }
    else
    {
        urls.append(urlInfo.value("url").toString());
    }
    if (urls.isEmpty())
    {
        lua_pushboolean(L, false);
        lua_pushstring(L, "invalid url");
        return 2;
    }
    const QString saveDir = urlInfo.value("save_dir", "").toString();
    bool skipMagnetConfirm = urlInfo.value("skip_magnet_confirm", false).toBool();
    bool skipConfirm = urlInfo.value("skip_confirm", false).toBool();
    QVariantList errInfo;
    bool userCancel = false;
    QMetaObject::invokeMethod(GlobalObjects::downloadModel, [&](){
        if (saveDir.isEmpty() || !skipConfirm)
        {
            AddUriTask addUriTaskDialog(app->window()->getWidget(), urls, saveDir);
            if (QDialog::Accepted == addUriTaskDialog.exec())
            {
                for (const QString &uri : addUriTaskDialog.uriList)
                {
                    QString err = GlobalObjects::downloadModel->addUriTask(uri, addUriTaskDialog.dir, skipMagnetConfirm);
                    if (!err.isEmpty())
                    {
                        errInfo.append(QStringList{uri, err});
                    }
                }
            }
            else
            {
               userCancel = true;
            }
        }
        else
        {
            for (const QString &url : urls)
            {
                QString err = GlobalObjects::downloadModel->addUriTask(url, saveDir, skipMagnetConfirm);
                if (!err.isEmpty())
                {
                    errInfo.append(QStringList{url, err});
                }
            }
        }
    }, Qt::BlockingQueuedConnection);
    lua_pushboolean(L, errInfo.isEmpty() && !userCancel);
    if (userCancel)
    {
        lua_pushstring(L, "user cancel");
    }
    else
    {
        if (errInfo.isEmpty()) lua_pushstring(L, "");
        else if (errInfo.size() == 1) lua_pushstring(L, errInfo.first().toStringList()[1].toUtf8().constData());
        else pushValue(L, errInfo);
    }
    return 2;
}

int DownloadInterface::addtorrent(lua_State *L)
{
    KApp *app = KApp::getApp(L);
    const int params = lua_gettop(L);
    if (!app || !app->window() || params < 1) return 0;
    size_t len = 0;
    const char *data = lua_tolstring(L, 1, &len);
    QByteArray torrentData(data, len);
    QString saveDir;
    if (params > 1) saveDir = QString(lua_tostring(L, 2)).trimmed();
    QString errInfo;
    QMetaObject::invokeMethod(GlobalObjects::downloadModel, [&](){
        try
        {
            TorrentDecoder decoder(torrentData);
            SelectTorrentFile selectTorrentFile(decoder.root, app->window()->getWidget(), saveDir);
            if(QDialog::Accepted == selectTorrentFile.exec())
            {
                errInfo = GlobalObjects::downloadModel->addTorrentTask(decoder.rawContent,decoder.infoHash,
                                                             selectTorrentFile.dir,selectTorrentFile.selectIndexes,QString());
            }
            else
            {
                errInfo = "user canceled";
            }
            delete decoder.root;
        }
        catch(TorrentError &err)
        {
            errInfo = err.errorInfo;
        }

    }, Qt::BlockingQueuedConnection);
    lua_pushboolean(L, errInfo.isEmpty());
    lua_pushstring(L, errInfo.toUtf8().constData());
    return 2;
}


}
