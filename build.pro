TEMPLATE = subdirs
SUBDIRS = main
main.file = KikoPlay.pro

!macx {
    SUBDIRS += Extension/Lua
    main.depends = Extension/Lua
}
