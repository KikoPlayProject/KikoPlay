#ifndef MPVLOG_H
#define MPVLOG_H

#include "framelessdialog.h"
class QComboBox;
class LogWindow : public CFramelessDialog
{
    Q_OBJECT
public:
    enum LogType
    {
        MPV, SCRIPT
    };
    explicit LogWindow(QWidget *parent = nullptr);

    void show(LogType lt = LogType::MPV);
private:
    QComboBox *logTypeCombo;
};

#endif // MPVLOG_H
