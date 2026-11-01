QT += core network xml testlib
QT -= gui
CONFIG += console testcase c++17
CONFIG -= app_bundle
TEMPLATE = app
TARGET = tst_webdavsubtitleloader

INCLUDEPATH += ../..
msvc:QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    tst_webdavsubtitleloader.cpp \
    ../../Play/Subtitle/webdavsubtitleloader.cpp \
    ../../Play/Playlist/webdav/qwebdav.cpp \
    ../../Play/Playlist/webdav/qwebdavdirparser.cpp \
    ../../Play/Playlist/webdav/qwebdavitem.cpp

HEADERS += \
    ../../Play/Subtitle/webdavsubtitleloader.h \
    ../../Play/Playlist/webdav/qwebdav.h \
    ../../Play/Playlist/webdav/qwebdavdirparser.h
