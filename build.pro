TEMPLATE = subdirs
SUBDIRS = main
main.file = KikoPlay.pro

!macx {
    SUBDIRS += Extension/lua
    main.depends = Extension/lua
}
