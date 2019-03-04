#ifndef MANAGESCRIPT_H
#define MANAGESCRIPT_H
#include "framelessdialog.h"
class ScriptManager;
class ManageScript : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit ManageScript(ScriptManager *manager, QWidget *parent = nullptr);
};

#endif // MANAGESCRIPT_H
