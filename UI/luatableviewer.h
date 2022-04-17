#ifndef LUATABLEVIEWER_H
#define LUATABLEVIEWER_H
#include "framelessdialog.h"
class LuaTableModel;
class LuaTableViewer : public CFramelessDialog
{
    Q_OBJECT
public:
    LuaTableViewer(LuaTableModel *model, QWidget *parent = nullptr);
private:
    int currentColumn;
};

#endif // LUATABLEVIEWER_H
