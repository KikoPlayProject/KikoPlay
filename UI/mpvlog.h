#ifndef MPVLOG_H
#define MPVLOG_H

#include "framelessdialog.h"

class MPVLog : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit MPVLog(QWidget *parent = nullptr);
};

#endif // MPVLOG_H
