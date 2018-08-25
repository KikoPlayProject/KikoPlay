#ifndef ABOUT_H
#define ABOUT_H

#include "framelessdialog.h"

class About : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit About(QWidget *parent = nullptr);
};

#endif // ABOUT_H
