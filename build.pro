TEMPLATE = subdirs
SUBDIRS = main
main.file = KikoPlay.pro

!macx {
    SUBDIRS += Script/lua
    main.depends = Script/lua
}
