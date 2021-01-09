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

TRANSLATIONS += res/lang/zh_CN.ts

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

SOURCES += \
    Common/notifier.cpp \
    Download/autodownloadmanager.cpp \
    Download/peermodel.cpp \
    LANServer/mediahandler.cpp \
    MediaLibrary/Service/bangumi.cpp \
    Play/Danmu/eventanalyzer.cpp \
    Play/Video/mpvpreview.cpp \
    Script/danmuscript.cpp \
    Script/libraryscript.cpp \
    Script/resourcescript.cpp \
    Script/scriptbase.cpp \
    Script/scriptmanager.cpp \
    UI/addpool.cpp \
    UI/addrule.cpp \
    UI/animedetailinfopage.cpp \
    UI/autodownloadwindow.cpp \
    UI/danmuview.cpp \
    UI/inputdialog.cpp \
    UI/settings.cpp \
    UI/settings/downloadpage.cpp \
    UI/settings/lanserverpage.cpp \
    UI/settings/mpvpage.cpp \
    UI/settings/scriptpage.cpp \
    UI/settings/stylepage.cpp \
    UI/stylemanager.cpp \
    UI/widgets/backgroundwidget.cpp \
    UI/widgets/clickslider.cpp \
    UI/widgets/colorslider.cpp \
    UI/widgets/danmustatiswidget.cpp \
    UI/widgets/dialogtip.cpp \
    UI/widgets/fonticonbutton.cpp \
        main.cpp \
    UI/mainwindow.cpp \
    UI/framelesswindow.cpp \
    Play/Danmu/Layouts/bottomlayout.cpp \
    Play/Danmu/Layouts/rolllayout.cpp \
    Play/Danmu/Layouts/toplayout.cpp \
    Play/Danmu/danmupool.cpp \
    globalobjects.cpp \
    Play/Playlist/playlist.cpp \
    Play/Video/mpvplayer.cpp \
    UI/list.cpp \
    UI/player.cpp \
    UI/pooleditor.cpp \
    UI/framelessdialog.cpp \
    Play/Danmu/Provider/localprovider.cpp \
    UI/adddanmu.cpp \
    UI/matcheditor.cpp \
    Play/Danmu/Provider/bilibiliprovider.cpp \
    UI/selectepisode.cpp \
    Play/Danmu/Provider/dandanprovider.cpp \
    Play/Danmu/blocker.cpp \
    UI/blockeditor.cpp \
    UI/capture.cpp \
    UI/mediainfo.cpp \
    Play/Danmu/common.cpp \
    UI/about.cpp \
    Play/Danmu/Provider/tucaoprovider.cpp \
    Play/Danmu/providermanager.cpp \
    Play/Danmu/Provider/bahamutprovider.cpp \
    Play/Danmu/Provider/dililiprovider.cpp \
    MediaLibrary/animelibrary.cpp \
    Common/network.cpp \
    Common/htmlparsersax.cpp \
    MediaLibrary/animeitemdelegate.cpp \
    UI/librarywindow.cpp \
    UI/bangumisearch.cpp \
    MediaLibrary/episodesmodel.cpp \
    Download/util.cpp \
    Download/aria2jsonrpc.cpp \
    UI/widgets/dirselectwidget.cpp \
    Download/downloaditemdelegate.cpp \
    Download/downloadmodel.cpp \
    Download/torrent.cpp \
    UI/downloadwindow.cpp \
    UI/adduritask.cpp \
    UI/selecttorrentfile.cpp \
    UI/poolmanager.cpp \
    UI/checkupdate.cpp \
    Play/Danmu/Provider/iqiyiprovider.cpp \
    Common/flowlayout.cpp \
    UI/animedetailinfo.cpp \
    UI/timelineedit.cpp \
    Play/Danmu/Provider/acfunprovider.cpp \
    UI/mpvlog.cpp \
    LANServer/lanserver.cpp \
    LANServer/httpserver.cpp \
    Play/Playlist/playlistitem.cpp \
    Play/Playlist/playlistprivate.cpp \
    Play/Danmu/Render/cacheworker.cpp \
    Play/Danmu/Render/danmurender.cpp \
    Play/Danmu/Manager/danmumanager.cpp \
    Play/Danmu/Manager/nodeinfo.cpp \
    Play/Danmu/Manager/managermodel.cpp \
    MediaLibrary/animeworker.cpp \
    MediaLibrary/animemodel.cpp \
    MediaLibrary/labelmodel.cpp \
    MediaLibrary/animefilterproxymodel.cpp \
    MediaLibrary/labelitemdelegate.cpp \
    UI/bangumiupdate.cpp \
    Play/Danmu/Provider/youkuprovider.cpp \
    Play/Danmu/Provider/tencentprovider.cpp \
    Download/BgmList/bgmlist.cpp \
    UI/bgmlistwindow.cpp \
    Download/Script/scriptmanager.cpp \
    UI/ressearchwindow.cpp \
    Play/Danmu/Provider/pptvprovider.cpp \
    Play/Danmu/Manager/pool.cpp \
    MediaLibrary/capturelistmodel.cpp \
    UI/captureview.cpp \
    UI/tip.cpp

HEADERS += \
    Common/kcache.h \
    Common/notifier.h \
    Download/autodownloadmanager.h \
    Download/peerid.h \
    Download/peermodel.h \
    LANServer/mediahandler.h \
    MediaLibrary/Service/bangumi.h \
    Play/Danmu/danmuviewmodel.h \
    Play/Danmu/eventanalyzer.h \
    Play/Video/mpvpreview.h \
    Script/danmuscript.h \
    Script/libraryscript.h \
    Script/resourcescript.h \
    Script/scriptbase.h \
    Script/scriptmanager.h \
    UI/addpool.h \
    UI/addrule.h \
    UI/animedetailinfopage.h \
    UI/autodownloadwindow.h \
    UI/danmuview.h \
    UI/inputdialog.h \
    UI/mainwindow.h \
    UI/framelesswindow.h \
    Play/Danmu/Layouts/bottomlayout.h \
    Play/Danmu/Layouts/danmulayout.h \
    Play/Danmu/Layouts/rolllayout.h \
    Play/Danmu/Layouts/toplayout.h \
    Play/Danmu/danmupool.h \
    UI/settings.h \
    UI/settings/downloadpage.h \
    UI/settings/lanserverpage.h \
    UI/settings/mpvpage.h \
    UI/settings/scriptpage.h \
    UI/settings/settingpage.h \
    UI/settings/stylepage.h \
    UI/stylemanager.h \
    UI/widgets/backgroundwidget.h \
    UI/widgets/clickslider.h \
    UI/widgets/colorslider.h \
    UI/widgets/danmustatiswidget.h \
    UI/widgets/dialogtip.h \
    UI/widgets/fonticonbutton.h \
    globalobjects.h \
    Play/Playlist/playlist.h \
    Play/Video/mpvplayer.h \
    UI/list.h \
    UI/player.h \
    UI/pooleditor.h \
    UI/framelessdialog.h \
    Play/Danmu/Provider/localprovider.h \
    UI/adddanmu.h \
    Play/Danmu/common.h \
    UI/matcheditor.h \
    Play/Danmu/Provider/bilibiliprovider.h \
    Play/Danmu/Provider/info.h \
    UI/selectepisode.h \
    Play/Danmu/Provider/dandanprovider.h \
    Play/Danmu/blocker.h \
    UI/blockeditor.h \
    UI/capture.h \
    UI/mediainfo.h \
    UI/about.h \
    Play/Danmu/Provider/tucaoprovider.h \
    Play/Danmu/Provider/providerbase.h \
    Play/Danmu/providermanager.h \
    Play/Danmu/Provider/bahamutprovider.h \
    Play/Danmu/Provider/dililiprovider.h \
    MediaLibrary/animelibrary.h \
    Common/network.h \
    Common/htmlparsersax.h \
    MediaLibrary/animeinfo.h \
    MediaLibrary/animeitemdelegate.h \
    UI/librarywindow.h \
    UI/bangumisearch.h \
    MediaLibrary/episodesmodel.h \
    Download/util.h \
    Download/aria2jsonrpc.h \
    UI/widgets/dirselectwidget.h \
    Download/downloaditemdelegate.h \
    Download/downloadmodel.h \
    Download/torrent.h \
    UI/downloadwindow.h \
    UI/adduritask.h \
    UI/selecttorrentfile.h \
    UI/poolmanager.h \
    UI/checkupdate.h \
    Play/Danmu/Provider/iqiyiprovider.h \
    Common/zconf.h \
    Common/zlib.h \
    Common/flowlayout.h \
    UI/animedetailinfo.h \
    UI/timelineedit.h \
    Play/Danmu/Provider/acfunprovider.h \
    UI/mpvlog.h \
    LANServer/lanserver.h \
    LANServer/httpserver.h \
    Play/Playlist/playlistitem.h \
    Play/Playlist/playlistprivate.h \
    Play/Danmu/Render/cacheworker.h \
    Play/Danmu/Render/danmurender.h \
    Play/Danmu/Manager/danmumanager.h \
    Play/Danmu/Manager/nodeinfo.h \
    Play/Danmu/Manager/managermodel.h \
    MediaLibrary/animeworker.h \
    MediaLibrary/animemodel.h \
    MediaLibrary/labelmodel.h \
    MediaLibrary/animefilterproxymodel.h \
    MediaLibrary/labelitemdelegate.h \
    UI/bangumiupdate.h \
    Play/Danmu/Provider/youkuprovider.h \
    Play/Danmu/Provider/tencentprovider.h \
    Download/BgmList/bgmlist.h \
    UI/bgmlistwindow.h \
    Download/Script/scriptmanager.h \
    UI/ressearchwindow.h \
    Play/Danmu/Provider/pptvprovider.h \
    Play/Danmu/Manager/pool.h \
    Common/threadtask.h \
    MediaLibrary/capturelistmodel.h \
    UI/captureview.h \
    UI/tip.h

INCLUDEPATH += \
    Play/Video \
    LANServer \
    Download/Script/lua
RESOURCES += \
    res.qrc

# Windows related settings
win32 {
    QMAKE_LFLAGS_RELEASE += /MAP
    QMAKE_CFLAGS_RELEASE += /Zi
    QMAKE_LFLAGS_RELEASE += /debug /opt:ref

    # Link library settings by ARCH
    contains(QT_ARCH, i386) {
        LIBS += -L$$PWD/lib/ -llibmpv.dll
        LIBS += -L$$PWD/lib/ -lzlibstat
        LIBS += -L$$PWD/lib/ -lqhttpengine
    } else {
        LIBS += -L$$PWD/lib/x64/ -llibmpv.dll
        LIBS += -L$$PWD/lib/x64/ -lzlibstat
        LIBS += -L$$PWD/lib/x64/ -lqhttpengine
        LIBS += -L$$PWD/lib/x64/ -llua53
    }
}

# UNIX related settings
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

    # Link library settings by ARCH
    contains(QT_ARCH, i386) {
        LIBS += -L/usr/lib/ -L$$PWD/lib/linux
    } else {
        LIBS += -L/usr/lib64/ -L$$PWD/lib/x64/linux -L$$PWD/lib64/linux
    }

    LIBS += -lmpv
    LIBS += -lz
    LIBS += -lm
    LIBS += -llua5.3
    LIBS += -lqhttpengine
    LIBS += -ldl
}
