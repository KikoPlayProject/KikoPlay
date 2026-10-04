#ifndef SPONSORDIALOG_H
#define SPONSORDIALOG_H

#include "framelessdialog.h"

class SponsorDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit SponsorDialog(QWidget *parent = nullptr);
};

#endif // SPONSORDIALOG_H
