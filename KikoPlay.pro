#-------------------------------------------------
#
# Project created by QtCreator 2018-05-29T10:46:56
#
#-------------------------------------------------

QT       += core gui sql network

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
        main.cpp \
    UI/mainwindow.cpp \
    UI/framelesswindow.cpp \
    Play/Danmu/Layouts/bottomlayout.cpp \
    Play/Danmu/Layouts/rolllayout.cpp \
    Play/Danmu/Layouts/toplayout.cpp \
    Play/Danmu/danmupool.cpp \
    Play/Danmu/danmurender.cpp \
    globalobjects.cpp \
    Play/Playlist/playlist.cpp \
    Play/Video/mpvplayer.cpp \
    UI/list.cpp \
    UI/player.cpp \
    UI/pooleditor.cpp \
    UI/framelessdialog.cpp \
    Play/Danmu/Provider/localprovider.cpp \
    UI/adddanmu.cpp \
    Play/Danmu/Provider/matchprovider.cpp \
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
    Download/dirselectwidget.cpp \
    Download/downloaditemdelegate.cpp \
    Download/downloadmodel.cpp \
    Download/torrent.cpp \
    UI/downloadwindow.cpp \
    UI/adduritask.cpp \
    UI/selecttorrentfile.cpp \
    UI/downloadsetting.cpp \
    Play/Danmu/danmumanager.cpp \
    UI/poolmanager.cpp \
    UI/checkupdate.cpp \
    Play/Danmu/Provider/iqiyiprovider.cpp \
    Common/flowlayout.cpp \
    UI/animedetailinfo.cpp \
    UI/timelineedit.cpp \
    Play/Danmu/Provider/acfunprovider.cpp \
    UI/mpvparametersetting.cpp \
    UI/mpvlog.cpp

HEADERS += \
    UI/mainwindow.h \
    UI/framelesswindow.h \
    Play/Danmu/Layouts/bottomlayout.h \
    Play/Danmu/Layouts/danmulayout.h \
    Play/Danmu/Layouts/rolllayout.h \
    Play/Danmu/Layouts/toplayout.h \
    Play/Danmu/danmupool.h \
    Play/Danmu/danmurender.h \
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
    Play/Danmu/Provider/matchprovider.h \
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
    Download/dirselectwidget.h \
    Download/downloaditemdelegate.h \
    Download/downloadmodel.h \
    Download/torrent.h \
    UI/downloadwindow.h \
    UI/adduritask.h \
    UI/selecttorrentfile.h \
    UI/downloadsetting.h \
    Play/Danmu/danmumanager.h \
    UI/poolmanager.h \
    UI/checkupdate.h \
    Play/Danmu/Provider/iqiyiprovider.h \
    Common/zconf.h \
    Common/zlib.h \
    Common/flowlayout.h \
    UI/animedetailinfo.h \
    UI/timelineedit.h \
    Play/Danmu/Provider/acfunprovider.h \
    UI/mpvparametersetting.h \
    UI/mpvlog.h

INCLUDEPATH += \
    Play/Video
RESOURCES += \
    res.qrc

contains(QT_ARCH, i386){
    win32: LIBS += -L$$PWD/lib/ -llibmpv.dll
    win32: LIBS += -L$$PWD/lib/ -lzlibstat
}else{
    win32: LIBS += -L$$PWD/lib/x64/ -llibmpv.dll
    win32: LIBS += -L$$PWD/lib/x64/ -lzlibstat
}




