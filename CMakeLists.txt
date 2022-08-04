cmake_minimum_required(VERSION 3.16.0)

# If QT is installed in your system, it can be FALSE
option(USE_VCPKG_QT "Use vcpkg to add QT dependency" TRUE)

if (USE_VCPKG_QT)
    list(APPEND VCPKG_MANIFEST_FEATURES "qt-dependencies")
endif()


project(KikoPlay)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Core Gui Network Concurrent Sql REQUIRED)
find_package(ZLIB REQUIRED)
add_subdirectory(Script/lua)

add_executable(${PROJECT_NAME}
    Common/counter.cpp 
    Common/logger.cpp 
    Common/notifier.cpp 
    Download/autodownloadmanager.cpp 
    Download/peermodel.cpp 
    Download/trackersubscriber.cpp 
    LANServer/apihandler.cpp 
    LANServer/dlna/upnpctrlpoint.cpp 
    LANServer/filehandler.cpp 
    LANServer/httpserver/httpconnectionhandler.cpp 
    LANServer/httpserver/httpconnectionhandlerpool.cpp 
    LANServer/httpserver/httpcookie.cpp 
    LANServer/httpserver/httpglobal.cpp 
    LANServer/httpserver/httplistener.cpp 
    LANServer/httpserver/httprequest.cpp 
    LANServer/httpserver/httprequesthandler.cpp 
    LANServer/httpserver/httpresponse.cpp 
    LANServer/httpserver/httpsession.cpp 
    LANServer/httpserver/httpsessionstore.cpp 
    LANServer/httpserver/staticfilecontroller.cpp 
    LANServer/router.cpp 
    MediaLibrary/animeinfo.cpp 
    MediaLibrary/animelistmodel.cpp 
    MediaLibrary/animeprovider.cpp 
    MediaLibrary/episodeitem.cpp 
    MediaLibrary/tagnode.cpp 
    Play/Danmu/danmuprovider.cpp 
    Play/Danmu/eventanalyzer.cpp 
    Play/Video/mpvpreview.cpp 
    Play/Video/simpleplayer.cpp 
    Script/bgmcalendarscript.cpp 
    Script/danmuscript.cpp 
    Script/libraryscript.cpp 
    Script/luatablemodel.cpp 
    Script/modules/lua_htmlparser.cpp 
    Script/modules/lua_net.cpp 
    Script/modules/lua_regex.cpp 
    Script/modules/lua_util.cpp 
    Script/modules/lua_xmlreader.cpp 
    Script/modules/modulebase.cpp 
    Script/playgroundscript.cpp 
    Script/resourcescript.cpp 
    Script/scriptbase.cpp 
    Script/scriptmanager.cpp 
    Script/scriptmodel.cpp 
    Script/scriptsettingmodel.cpp 
    UI/addpool.cpp 
    UI/addrule.cpp 
    UI/animebatchaction.cpp 
    UI/animedetailinfopage.cpp 
    UI/animeinfoeditor.cpp 
    UI/animesearch.cpp 
    UI/autodownloadwindow.cpp 
    UI/charactereditor.cpp 
    UI/danmulaunch.cpp 
    UI/danmuview.cpp 
    UI/dlnadiscover.cpp 
    UI/gifcapture.cpp 
    UI/inputdialog.cpp 
    UI/logwindow.cpp 
    UI/luatableviewer.cpp 
    UI/scriptplayground.cpp 
    UI/settings.cpp 
    UI/settings/downloadpage.cpp 
    UI/settings/lanserverpage.cpp 
    UI/settings/mpvpage.cpp 
    UI/settings/mpvshortcutpage.cpp 
    UI/settings/scriptpage.cpp 
    UI/settings/stylepage.cpp 
    UI/settings/settingpage.cpp
    UI/snippetcapture.cpp 
    UI/stylemanager.cpp 
    UI/widgets/backgroundfadewidget.cpp 
    UI/widgets/backgroundwidget.cpp 
    UI/widgets/clickslider.cpp 
    UI/widgets/colorpicker.cpp 
    UI/widgets/colorslider.cpp 
    UI/widgets/danmustatiswidget.cpp 
    UI/widgets/dialogtip.cpp 
    UI/widgets/fonticonbutton.cpp 
    UI/widgets/loadingicon.cpp 
        main.cpp 
    UI/mainwindow.cpp 
    UI/framelesswindow.cpp 
    Play/Danmu/Layouts/bottomlayout.cpp 
    Play/Danmu/Layouts/rolllayout.cpp 
    Play/Danmu/Layouts/toplayout.cpp 
    Play/Danmu/danmupool.cpp 
    globalobjects.cpp 
    Play/Playlist/playlist.cpp 
    Play/Video/mpvplayer.cpp 
    UI/list.cpp 
    UI/player.cpp 
    UI/pooleditor.cpp 
    UI/framelessdialog.cpp 
    Play/Danmu/Provider/localprovider.cpp 
    UI/adddanmu.cpp 
    UI/matcheditor.cpp 
    UI/selectepisode.cpp 
    Play/Danmu/blocker.cpp 
    UI/blockeditor.cpp 
    UI/capture.cpp 
    UI/mediainfo.cpp 
    Play/Danmu/common.cpp 
    UI/about.cpp 
    Common/network.cpp 
    Common/htmlparsersax.cpp 
    MediaLibrary/animeitemdelegate.cpp 
    UI/librarywindow.cpp 
    MediaLibrary/episodesmodel.cpp 
    Download/util.cpp 
    Download/aria2jsonrpc.cpp 
    UI/widgets/dirselectwidget.cpp 
    Download/downloaditemdelegate.cpp 
    Download/downloadmodel.cpp 
    Download/torrent.cpp 
    UI/downloadwindow.cpp 
    UI/adduritask.cpp 
    UI/selecttorrentfile.cpp 
    UI/poolmanager.cpp 
    UI/checkupdate.cpp 
    Common/flowlayout.cpp 
    UI/timelineedit.cpp 
    LANServer/lanserver.cpp 
    Play/Playlist/playlistitem.cpp 
    Play/Playlist/playlistprivate.cpp 
    Play/Danmu/Render/cacheworker.cpp 
    Play/Danmu/Render/danmurender.cpp 
    Play/Danmu/Manager/danmumanager.cpp 
    Play/Danmu/Manager/nodeinfo.cpp 
    Play/Danmu/Manager/managermodel.cpp 
    MediaLibrary/animeworker.cpp 
    MediaLibrary/animemodel.cpp 
    MediaLibrary/labelmodel.cpp 
    MediaLibrary/animefilterproxymodel.cpp 
    MediaLibrary/labelitemdelegate.cpp 
    Download/BgmList/bgmlist.cpp 
    UI/bgmlistwindow.cpp 
    UI/ressearchwindow.cpp 
    Play/Danmu/Manager/pool.cpp 
    MediaLibrary/capturelistmodel.cpp 
    UI/captureview.cpp 
    UI/tip.cpp
    res.qrc
    kikoplay.rc
)

target_include_directories(${PROJECT_NAME}
PRIVATE 
    Common
    Download
    LANServer
    MediaLibrary
    Play
    Script
    UI
    UI/settings/
    .
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(${PROJECT_NAME} 
PRIVATE 
    ZLIB::ZLIB
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Network 
    Qt::Concurrent 
    Qt::Sql
    myLua53
)

if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UNICODE)
    find_package(Qt5WinExtras CONFIG REQUIRED)

    target_include_directories(${PROJECT_NAME}
    PRIVATE
        Play/Video
    )
    target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        Qt::WinExtras
        ${PROJECT_SOURCE_DIR}/lib/x64/libmpv.dll.lib
    )
endif()

if (UNIX)
    include(FindPkgConfig)
    pkg_check_modules(mpv QUIET mpv)

    if (NOT mpv_FOUND)
        message(FATAL_ERROR "libmpv is not found in the system. Use 'sudo apt install libmpv-dev' to install(Ubuntu)")
    endif()

    target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        ${mpv_LIBRARIES}
    )
endif()

