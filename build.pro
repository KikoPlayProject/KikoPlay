TEMPLATE = subdirs
SUBDIRS = main
main.file = KikoPlay.pro

SUBDIRS += Extension/Lua
main.depends = Extension/Lua
