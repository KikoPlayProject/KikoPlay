cmake_minimum_required(VERSION 3.16.0)

# If QT is installed in your system, it can be FALSE
option(USE_VCPKG_QT "Use vcpkg to add QT dependency" ON)

if (USE_VCPKG_QT)
    list(APPEND VCPKG_MANIFEST_FEATURES "qt-dependencies")
endif()


project(KikoPlay)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Widgets Core Gui Network Concurrent Sql Svg REQUIRED)
find_package(ZLIB REQUIRED)
add_subdirectory(Script/lua)

function(include_sub_directories_recursively ROOT_DIR)
    if (IS_DIRECTORY ${ROOT_DIR})
        include_directories(${ROOT_DIR})
    endif()

    file(GLOB SUB_LIST RELATIVE ${ROOT_DIR} ${ROOT_DIR}/*)
    foreach(SUB ${SUB_LIST})
        if (IS_DIRECTORY ${ROOT_DIR}/${SUB})
            include_sub_directories_recursively(${ROOT_DIR}/${SUB})
        endif()
    endforeach()
endfunction()

set (CMAKE_PROJECT_SEARCH_PATH
    ${CMAKE_SOURCE_DIR}/Common
    ${CMAKE_SOURCE_DIR}/Download
    ${CMAKE_SOURCE_DIR}/LANServer
    ${CMAKE_SOURCE_DIR}/MediaLibrary
    ${CMAKE_SOURCE_DIR}/Play
    ${CMAKE_SOURCE_DIR}/Script
    ${CMAKE_SOURCE_DIR}/UI
)

foreach(SEARCH_PATH ${CMAKE_PROJECT_SEARCH_PATH})
    include_sub_directories_recursively(${SEARCH_PATH})
endforeach()

file(READ "KikoPlay.pro" KIKOPLAY_PRO_CONTENTS)

string(REGEX MATCHALL "[A-Za-z0-9_\\/]+\\.cpp" CMAKE_PROJECT_SOURCE_FILES "${KIKOPLAY_PRO_CONTENTS}")
message("add sources: " "${CMAKE_PROJECT_SOURCE_FILES}")

add_executable(${PROJECT_NAME}
    ${CMAKE_PROJECT_SOURCE_FILES}
    res.qrc
    kikoplay.rc
)

target_include_directories(${PROJECT_NAME}
PRIVATE
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
    Qt::Svg
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
	find_package(Qt5 COMPONENTS DBus REQUIRED)
    # Set default overridable parameters for "/usr/share"
    set(CMAKE_INSTALL_SHAREDIR "/usr/share" CACHE STRING "The default share path")
    # Set default unix data option
    option(CONFIG_UNIX_DATA "Use unix data path" ON)

    include(FindPkgConfig)
    pkg_check_modules(mpv QUIET mpv)

    if (NOT mpv_FOUND)
        message(FATAL_ERROR "libmpv is not found in the system. Use 'sudo apt install libmpv-dev' to install(Ubuntu)")
    endif()

    target_link_libraries(${PROJECT_NAME} 
    PRIVATE
        ${mpv_LIBRARIES}
		Qt5::DBus
    )

    if (CONFIG_UNIX_DATA)
        message("CONFIG_UNIX_DATA = ON")
        target_compile_definitions(${PROJECT_NAME} PRIVATE CONFIG_UNIX_DATA=1)
    endif()

    install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
    install(FILES kikoplay.png kikoplay.xpm DESTINATION "${CMAKE_INSTALL_SHAREDIR}/pixmaps")
    install(FILES kikoplay.desktop DESTINATION "${CMAKE_INSTALL_SHAREDIR}/applications")
    install(DIRECTORY web DESTINATION "${CMAKE_INSTALL_SHAREDIR}/kikoplay")
endif()
