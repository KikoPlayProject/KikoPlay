#-------------------------------------------------
#
# Project created by QtCreator 2018-05-29T10:46:56
#
#-------------------------------------------------

QT       += core gui sql network concurrent websockets
linux:QT += dbus
win32:QT += winextras

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = KikoPlay
TEMPLATE = app
RC_FILE += kikoplay.rc
RC_ICONS = kikoplay.ico

TRANSLATIONS += res/lang/zh_Hans.ts
DEFINES += QT_MESSAGELOGCONTEXT
# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
DEFINES += ZLIB_WINAPI
# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

CONFIG += C++11

CONFIG += debug_and_release
CONFIG(debug, debug|release) {
    TARGET = $${TARGET}.debug
#    QMAKE_CXX_FLAGS_MT_DBG += /Od /D_DEBUG
} else {
    TARGET = $${TARGET}
}

SOURCES += \
    Common/counter.cpp \
    Common/eventbus.cpp \
    Common/flowlayout.cpp \
    Common/htmlparsersax.cpp \
    Common/kstats.cpp \
    Common/kupdater.cpp \
    Common/logger.cpp \
    Common/network.cpp \
    Common/notifier.cpp \
    Download/aria2jsonrpc.cpp \
    Download/autodownloadmanager.cpp \
    Download/BgmList/bgmlist.cpp \
    Download/downloaditemdelegate.cpp \
    Download/downloadmodel.cpp \
    Download/peermodel.cpp \
    Download/torrent.cpp \
    Download/trackersubscriber.cpp \
    Extension/App/AppWidgets/appbutton.cpp \
    Extension/App/AppWidgets/appcheckbox.cpp \
    Extension/App/AppWidgets/appcombo.cpp \
    Extension/App/AppWidgets/appgview.cpp \
    Extension/App/AppWidgets/apphview.cpp \
    Extension/App/AppWidgets/applabel.cpp \
    Extension/App/AppWidgets/applist.cpp \
    Extension/App/AppWidgets/appprogress.cpp \
    Extension/App/AppWidgets/appradio.cpp \
    Extension/App/AppWidgets/appslider.cpp \
    Extension/App/AppWidgets/appsview.cpp \
    Extension/App/AppWidgets/apptextbox.cpp \
    Extension/App/AppWidgets/apptextline.cpp \
    Extension/App/AppWidgets/apptree.cpp \
    Extension/App/AppWidgets/apputil.cpp \
    Extension/App/AppWidgets/appview.cpp \
    Extension/App/AppWidgets/appvview.cpp \
    Extension/App/AppWidgets/appwidget.cpp \
    Extension/App/AppWidgets/appwindow.cpp \
    Extension/App/appframelessdialog.cpp \
    Extension/App/appmanager.cpp \
    Extension/App/appstorage.cpp \
    Extension/App/kapp.cpp \
    Extension/Common/ext_common.cpp \
    Extension/Common/luatablemodel.cpp \
    Extension/Modules/lua_appcommondialog.cpp \
    Extension/Modules/lua_appevent.cpp \
    Extension/Modules/lua_appimage.cpp \
    Extension/Modules/lua_appnet.cpp \
    Extension/Modules/lua_appui.cpp \
    Extension/Modules/lua_apputil.cpp \
    Extension/Modules/lua_clipboardinterface.cpp \
    Extension/Modules/lua_danmuinterface.cpp \
    Extension/Modules/lua_dir.cpp \
    Extension/Modules/lua_downloadinterface.cpp \
    Extension/Modules/lua_htmlparser.cpp \
    Extension/Modules/lua_libraryinterface.cpp \
    Extension/Modules/lua_net.cpp \
    Extension/Modules/lua_playerinterface.cpp \
    Extension/Modules/lua_playlistinterface.cpp \
    Extension/Modules/lua_process.cpp \
    Extension/Modules/lua_regex.cpp \
    Extension/Modules/lua_storageinterface.cpp \
    Extension/Modules/lua_stringutil.cpp \
    Extension/Modules/lua_timer.cpp \
    Extension/Modules/lua_util.cpp \
    Extension/Modules/lua_xmlreader.cpp \
    Extension/Modules/modulebase.cpp \
    Extension/Script/bgmcalendarscript.cpp \
    Extension/Script/danmuscript.cpp \
    Extension/Script/libraryscript.cpp \
    Extension/Script/playgroundscript.cpp \
    Extension/Script/resourcescript.cpp \
    Extension/Script/scriptbase.cpp \
    Extension/Script/scriptmanager.cpp \
    Extension/Script/scriptmodel.cpp \
    Extension/Script/scriptsettingmodel.cpp \
    Download/util.cpp \
    UI/settings/apppage.cpp \
    UI/widgets/windowtip.cpp \
    globalobjects.cpp \
    LANServer/apihandler.cpp \
    LANServer/dlna/dlnamediacontroller.cpp \
    LANServer/dlna/dlnamediaitem.cpp \
    LANServer/dlna/dlnamediaserver.cpp \
    LANServer/dlna/upnp.cpp \
    LANServer/dlna/upnpctrlpoint.cpp \
    LANServer/dlna/upnpdevice.cpp \
    LANServer/dlna/upnpservice.cpp \
    LANServer/filehandler.cpp \
    LANServer/httpserver/httpconnectionhandler.cpp \
    LANServer/httpserver/httpconnectionhandlerpool.cpp \
    LANServer/httpserver/httpcookie.cpp \
    LANServer/httpserver/httpglobal.cpp \
    LANServer/httpserver/httplistener.cpp \
    LANServer/httpserver/httprequest.cpp \
    LANServer/httpserver/httprequesthandler.cpp \
    LANServer/httpserver/httpresponse.cpp \
    LANServer/httpserver/httpsession.cpp \
    LANServer/httpserver/httpsessionstore.cpp \
    LANServer/httpserver/staticfilecontroller.cpp \
    LANServer/lanserver.cpp \
    LANServer/router.cpp \
    main.cpp \
    MediaLibrary/animefilterproxymodel.cpp \
    MediaLibrary/animeinfo.cpp \
    MediaLibrary/animeitemdelegate.cpp \
    MediaLibrary/animelistmodel.cpp \
    MediaLibrary/animemodel.cpp \
    MediaLibrary/animeprovider.cpp \
    MediaLibrary/animeworker.cpp \
    MediaLibrary/capturelistmodel.cpp \
    MediaLibrary/episodeitem.cpp \
    MediaLibrary/episodesmodel.cpp \
    MediaLibrary/labelitemdelegate.cpp \
    MediaLibrary/labelmodel.cpp \
    MediaLibrary/tagnode.cpp \
    Play/Danmu/blocker.cpp \
    Play/Danmu/common.cpp \
    Play/Danmu/danmupool.cpp \
    Play/Danmu/danmuprovider.cpp \
    Play/Danmu/eventanalyzer.cpp \
    Play/Danmu/Layouts/bottomlayout.cpp \
    Play/Danmu/Layouts/rolllayout.cpp \
    Play/Danmu/Layouts/toplayout.cpp \
    Play/Danmu/Manager/danmumanager.cpp \
    Play/Danmu/Manager/managermodel.cpp \
    Play/Danmu/Manager/nodeinfo.cpp \
    Play/Danmu/Manager/pool.cpp \
    Play/Danmu/Provider/localprovider.cpp \
    Play/Danmu/Render/cacheworker.cpp \
    Play/Danmu/Render/danmurender.cpp \
    Play/Danmu/Render/livedanmuitemdelegate.cpp \
    Play/Danmu/Render/livedanmulistmodel.cpp \
    Play/Playlist/playlist.cpp \
    Play/Playlist/playlistitem.cpp \
    Play/Playlist/playlistprivate.cpp \
    Play/Video/mpvplayer.cpp \
    Play/Video/mpvpreview.cpp \
    Play/Video/simpleplayer.cpp \
    Play/playcontext.cpp \
    UI/about.cpp \
    UI/adddanmu.cpp \
    UI/addpool.cpp \
    UI/addrule.cpp \
    UI/adduritask.cpp \
    UI/animebatchaction.cpp \
    UI/animedetailinfopage.cpp \
    UI/animeinfoeditor.cpp \
    UI/animesearch.cpp \
    UI/appbar.cpp \
    UI/appmenu.cpp \
    UI/autodownloadwindow.cpp \
    UI/bgmlistwindow.cpp \
    UI/blockeditor.cpp \
    UI/capture.cpp \
    UI/captureview.cpp \
    UI/charactereditor.cpp \
    UI/checkupdate.cpp \
    UI/danmulaunch.cpp \
    UI/danmuview.cpp \
    UI/dlnadiscover.cpp \
    UI/downloadwindow.cpp \
    UI/framelessdialog.cpp \
    UI/framelesswindow.cpp \
    UI/gifcapture.cpp \
    UI/inputdialog.cpp \
    UI/librarywindow.cpp \
    UI/list.cpp \
    UI/logwindow.cpp \
    UI/luatableviewer.cpp \
    UI/mainwindow.cpp \
    UI/matcheditor.cpp \
    UI/mediainfo.cpp \
    UI/player.cpp \
    UI/pooleditor.cpp \
    UI/poolmanager.cpp \
    UI/ressearchwindow.cpp \
    UI/scriptplayground.cpp \
    UI/selectepisode.cpp \
    UI/selecttorrentfile.cpp \
    UI/settings.cpp \
    UI/settings/downloadpage.cpp \
    UI/settings/lanserverpage.cpp \
    UI/settings/mpvpage.cpp \
    UI/settings/mpvshortcutpage.cpp \
    UI/settings/scriptpage.cpp \
    UI/settings/settingpage.cpp \
    UI/settings/stylepage.cpp \
    UI/snippetcapture.cpp \
    UI/stylemanager.cpp \
    UI/timelineedit.cpp \
    UI/tip.cpp \
    UI/widgets/backgroundfadewidget.cpp \
    UI/widgets/backgroundwidget.cpp \
    UI/widgets/clickslider.cpp \
    UI/widgets/colorpicker.cpp \
    UI/widgets/colorslider.cpp \
    UI/widgets/danmustatiswidget.cpp \
    UI/widgets/dialogtip.cpp \
    UI/widgets/dirselectwidget.cpp \
    UI/widgets/elidelineedit.cpp \
    UI/widgets/fonticonbutton.cpp \
    UI/widgets/loadingicon.cpp \
    UI/widgets/optionslider.cpp \
    UI/widgets/scriptsearchoptionpanel.cpp \
    UI/widgets/smoothscrollbar.cpp

HEADERS += \
    Common/counter.h \
    Common/eventbus.h \
    Common/flowlayout.h \
    Common/htmlparsersax.h \
    Common/kstats.h \
    Common/kupdater.h \
    Common/logger.h \
    Common/lrucache.h \
    Common/network.h \
    Common/notifier.h \
    Common/threadtask.h \
    Common/zconf.h \
    Common/zlib.h \
    Download/aria2jsonrpc.h \
    Download/autodownloadmanager.h \
    Download/BgmList/bgmlist.h \
    Download/downloaditemdelegate.h \
    Download/downloadmodel.h \
    Download/peerid.h \
    Download/peermodel.h \
    Download/torrent.h \
    Download/trackersubscriber.h \
    Extension/App/AppWidgets/appbutton.h \
    Extension/App/AppWidgets/appcheckbox.h \
    Extension/App/AppWidgets/appcombo.h \
    Extension/App/AppWidgets/appgview.h \
    Extension/App/AppWidgets/apphview.h \
    Extension/App/AppWidgets/applabel.h \
    Extension/App/AppWidgets/applist.h \
    Extension/App/AppWidgets/appprogress.h \
    Extension/App/AppWidgets/appradio.h \
    Extension/App/AppWidgets/appslider.h \
    Extension/App/AppWidgets/appsview.h \
    Extension/App/AppWidgets/apptextbox.h \
    Extension/App/AppWidgets/apptextline.h \
    Extension/App/AppWidgets/apptree.h \
    Extension/App/AppWidgets/apputil.h \
    Extension/App/AppWidgets/appview.h \
    Extension/App/AppWidgets/appvview.h \
    Extension/App/AppWidgets/appwidget.h \
    Extension/App/AppWidgets/appwindow.h \
    Extension/App/appframelessdialog.h \
    Extension/App/appmanager.h \
    Extension/App/appstorage.h \
    Extension/App/kapp.h \
    Extension/Common/ext_common.h \
    Extension/Common/luatablemodel.h \
    Extension/Modules/lua_appcommondialog.h \
    Extension/Modules/lua_appevent.h \
    Extension/Modules/lua_appimage.h \
    Extension/Modules/lua_appnet.h \
    Extension/Modules/lua_appui.h \
    Extension/Modules/lua_apputil.h \
    Extension/Modules/lua_clipboardinterface.h \
    Extension/Modules/lua_danmuinterface.h \
    Extension/Modules/lua_dir.h \
    Extension/Modules/lua_downloadinterface.h \
    Extension/Modules/lua_htmlparser.h \
    Extension/Modules/lua_libraryinterface.h \
    Extension/Modules/lua_net.h \
    Extension/Modules/lua_playerinterface.h \
    Extension/Modules/lua_playlistinterface.h \
    Extension/Modules/lua_process.h \
    Extension/Modules/lua_regex.h \
    Extension/Modules/lua_storageinterface.h \
    Extension/Modules/lua_stringutil.h \
    Extension/Modules/lua_timer.h \
    Extension/Modules/lua_util.h \
    Extension/Modules/lua_xmlreader.h \
    Extension/Modules/modulebase.h \
    Extension/Script/bgmcalendarscript.h \
    Extension/Script/danmuscript.h \
    Extension/Script/libraryscript.h \
    Extension/Script/playgroundscript.h \
    Extension/Script/resourcescript.h \
    Extension/Script/scriptbase.h \
    Extension/Script/scriptmanager.h \
    Extension/Script/scriptmodel.h \
    Extension/Script/scriptsettingmodel.h \
    Download/util.h \
    UI/settings/apppage.h \
    UI/widgets/windowtip.h \
    globalobjects.h \
    LANServer/apihandler.h \
    LANServer/dlna/dlnamediacontroller.h \
    LANServer/dlna/dlnamediaitem.h \
    LANServer/dlna/dlnamediaserver.h \
    LANServer/dlna/upnp.h \
    LANServer/dlna/upnpctrlpoint.h \
    LANServer/dlna/upnpdevice.h \
    LANServer/dlna/upnpservice.h \
    LANServer/filehandler.h \
    LANServer/httpserver/httpconnectionhandler.h \
    LANServer/httpserver/httpconnectionhandlerpool.h \
    LANServer/httpserver/httpcookie.h \
    LANServer/httpserver/httpglobal.h \
    LANServer/httpserver/httplistener.h \
    LANServer/httpserver/httprequest.h \
    LANServer/httpserver/httprequesthandler.h \
    LANServer/httpserver/httpresponse.h \
    LANServer/httpserver/httpsession.h \
    LANServer/httpserver/httpsessionstore.h \
    LANServer/httpserver/staticfilecontroller.h \
    LANServer/lanserver.h \
    LANServer/router.h \
    MediaLibrary/animefilterproxymodel.h \
    MediaLibrary/animeinfo.h \
    MediaLibrary/animeitemdelegate.h \
    MediaLibrary/animelistmodel.h \
    MediaLibrary/animemodel.h \
    MediaLibrary/animeprovider.h \
    MediaLibrary/animeworker.h \
    MediaLibrary/capturelistmodel.h \
    MediaLibrary/episodeitem.h \
    MediaLibrary/episodesmodel.h \
    MediaLibrary/labelitemdelegate.h \
    MediaLibrary/labelmodel.h \
    MediaLibrary/tagnode.h \
    Play/Danmu/blocker.h \
    Play/Danmu/common.h \
    Play/Danmu/danmupool.h \
    Play/Danmu/danmuprovider.h \
    Play/Danmu/danmuviewmodel.h \
    Play/Danmu/eventanalyzer.h \
    Play/Danmu/Layouts/bottomlayout.h \
    Play/Danmu/Layouts/danmulayout.h \
    Play/Danmu/Layouts/rolllayout.h \
    Play/Danmu/Layouts/toplayout.h \
    Play/Danmu/Manager/danmumanager.h \
    Play/Danmu/Manager/managermodel.h \
    Play/Danmu/Manager/nodeinfo.h \
    Play/Danmu/Manager/pool.h \
    Play/Danmu/Provider/localprovider.h \
    Play/Danmu/Render/cacheworker.h \
    Play/Danmu/Render/danmurender.h \
    Play/Danmu/Render/livedanmuitemdelegate.h \
    Play/Danmu/Render/livedanmulistmodel.h \
    Play/Playlist/playlist.h \
    Play/Playlist/playlistitem.h \
    Play/Playlist/playlistprivate.h \
    Play/Video/mpvplayer.h \
    Play/Video/mpvpreview.h \
    Play/Video/simpleplayer.h \
    Play/playcontext.h \
    UI/about.h \
    UI/adddanmu.h \
    UI/addpool.h \
    UI/addrule.h \
    UI/adduritask.h \
    UI/animebatchaction.h \
    UI/animedetailinfopage.h \
    UI/animeinfoeditor.h \
    UI/animesearch.h \
    UI/appbar.h \
    UI/appmenu.h \
    UI/autodownloadwindow.h \
    UI/bgmlistwindow.h \
    UI/blockeditor.h \
    UI/capture.h \
    UI/captureview.h \
    UI/charactereditor.h \
    UI/checkupdate.h \
    UI/danmulaunch.h \
    UI/danmuview.h \
    UI/dlnadiscover.h \
    UI/downloadwindow.h \
    UI/framelessdialog.h \
    UI/framelesswindow.h \
    UI/gifcapture.h \
    UI/inputdialog.h \
    UI/librarywindow.h \
    UI/list.h \
    UI/logwindow.h \
    UI/luatableviewer.h \
    UI/mainwindow.h \
    UI/matcheditor.h \
    UI/mediainfo.h \
    UI/player.h \
    UI/pooleditor.h \
    UI/poolmanager.h \
    UI/ressearchwindow.h \
    UI/scriptplayground.h \
    UI/selectepisode.h \
    UI/selecttorrentfile.h \
    UI/settings.h \
    UI/settings/downloadpage.h \
    UI/settings/lanserverpage.h \
    UI/settings/mpvpage.h \
    UI/settings/mpvshortcutpage.h \
    UI/settings/scriptpage.h \
    UI/settings/settingpage.h \
    UI/settings/stylepage.h \
    UI/snippetcapture.h \
    UI/stylemanager.h \
    UI/timelineedit.h \
    UI/tip.h \
    UI/widgets/backgroundfadewidget.h \
    UI/widgets/backgroundwidget.h \
    UI/widgets/clickslider.h \
    UI/widgets/colorpicker.h \
    UI/widgets/colorslider.h \
    UI/widgets/danmustatiswidget.h \
    UI/widgets/dialogtip.h \
    UI/widgets/dirselectwidget.h \
    UI/widgets/elidelineedit.h \
    UI/widgets/fonticonbutton.h \
    UI/widgets/loadingicon.h \
    UI/widgets/optionslider.h \
    UI/widgets/scriptsearchoptionpanel.h \
    UI/widgets/smoothscrollbar.h

INCLUDEPATH += \
    Play/Video \

RESOURCES += \
    res.qrc

# Windows related settings
win32 {
    QMAKE_LFLAGS_RELEASE += /MAP
    QMAKE_CFLAGS_RELEASE += /Zi
    QMAKE_CFLAGS_RELEASE += /MT
    QMAKE_LFLAGS_RELEASE += /debug /opt:ref

    # Link library settings by ARCH
    contains(QT_ARCH, i386) {
        LIBS += -L$$PWD/lib/ -llibmpv.dll
        LIBS += -L$$PWD/lib/ -lzlibstat
    } else {
        LIBS += -L$$PWD/lib/x64/ -llibmpv.dll
        LIBS += -L$$PWD/lib/x64/ -lzlibstat
        LIBS += -L$$PWD/lib/x64/ -llua53
    }
}

# UNIX related settings
macx {
    LIBS += -L/usr/lib -L/usr/local/lib -L/opt/local/lib -L$$PWD/lib/mac
    LIBS += -llua5.3
}

linux-g++* {
    QMAKE_LFLAGS += -fuse-ld=gold -Wl,--exclude-libs,liblua53.a

    # Link library settings by ARCH
    contains(QT_ARCH, i386) {
        LIBS += -L/usr/lib -L$$PWD/lib/linux
    } else {
        LIBS += -L/usr/lib64 -L/usr/lib/x86_64-linux-gnu -L$$PWD/lib/x64/linux
    }
    LIBS += -LExtension/Lua -l:liblua53.a
    LIBS += -lm -ldl
}

unix {
    # Install settings
    target.path += /usr/bin
    unix:icons.path = /usr/share/pixmaps
    unix:desktop.path = /usr/share/applications
    unix:icons.files = kikoplay.png kikoplay.xpm
    unix:desktop.files = kikoplay.desktop
    unix:web.path = /usr/share/kikoplay/web
    unix:web.files = web/*

    INSTALLS += target icons desktop web
    DEFINES += CONFIG_UNIX_DATA

    LIBS += -lmpv
    LIBS += -lz
}
