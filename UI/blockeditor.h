#ifndef BLOCKEDITOR_H
#define BLOCKEDITOR_H

#include "framelessdialog.h"

class QTreeView;
class BlockEditor : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit BlockEditor(QWidget *parent = nullptr);
    ~BlockEditor();
};

#endif // BLOCKEDITOR_H
