#-------------------------------------------------
#
# Project created by QtCreator 2018-05-29T10:46:56
#
#-------------------------------------------------

QT       += core gui sql network concurrent
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
    Common/flowlayout.cpp \
    Common/htmlparsersax.cpp \
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
    Download/util.cpp \
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
    Script/bgmcalendarscript.cpp \
    Script/danmuscript.cpp \
    Script/libraryscript.cpp \
    Script/luatablemodel.cpp \
    Script/modules/lua_htmlparser.cpp \
    Script/modules/lua_net.cpp \
    Script/modules/lua_regex.cpp \
    Script/modules/lua_util.cpp \
    Script/modules/lua_xmlreader.cpp \
    Script/modules/modulebase.cpp \
    Script/playgroundscript.cpp \
    Script/resourcescript.cpp \
    Script/scriptbase.cpp \
    Script/scriptmanager.cpp \
    Script/scriptmodel.cpp \
    Script/scriptsettingmodel.cpp \
    UI/about.cpp \
    UI/adddanmu.cpp \
    UI/addpool.cpp \
    UI/addrule.cpp \
    UI/adduritask.cpp \
    UI/animebatchaction.cpp \
    UI/animedetailinfopage.cpp \
    UI/animeinfoeditor.cpp \
    UI/animesearch.cpp \
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
    Common/flowlayout.h \
    Common/htmlparsersax.h \
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
    Download/util.h \
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
    Script/bgmcalendarscript.h \
    Script/danmuscript.h \
    Script/libraryscript.h \
    Script/luatablemodel.h \
    Script/modules/lua_htmlparser.h \
    Script/modules/lua_net.h \
    Script/modules/lua_regex.h \
    Script/modules/lua_util.h \
    Script/modules/lua_xmlreader.h \
    Script/modules/modulebase.h \
    Script/playgroundscript.h \
    Script/resourcescript.h \
    Script/scriptbase.h \
    Script/scriptmanager.h \
    Script/scriptmodel.h \
    Script/scriptsettingmodel.h \
    UI/about.h \
    UI/adddanmu.h \
    UI/addpool.h \
    UI/addrule.h \
    UI/adduritask.h \
    UI/animebatchaction.h \
    UI/animedetailinfopage.h \
    UI/animeinfoeditor.h \
    UI/animesearch.h \
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
    LIBS += -LScript/lua -l:liblua53.a
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
