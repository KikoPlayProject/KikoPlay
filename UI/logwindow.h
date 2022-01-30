#ifndef MPVLOG_H
#define MPVLOG_H

#include "framelessdialog.h"
#include "Common/logger.h"
class QComboBox;
class LogWindow : public CFramelessDialog
{
    Q_OBJECT
public:

    explicit LogWindow(QWidget *parent = nullptr);

    void show(Logger::LogType lt = Logger::LogType::APP);
private:
    QComboBox *logTypeCombo;
};

#endif // MPVLOG_H
