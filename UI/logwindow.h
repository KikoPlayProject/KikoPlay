#ifndef MPVLOG_H
#define MPVLOG_H

#include "framelessdialog.h"
#include "Common/logger.h"
class QComboBox;
class MPVPropertyViewer : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit MPVPropertyViewer(QWidget *parent = nullptr);
};
class LogWindow : public CFramelessDialog
{
    Q_OBJECT
public:

    explicit LogWindow(QWidget *parent = nullptr);

    void show(Logger::LogType lt = Logger::LogType::APP);
private:
    QComboBox *logTypeCombo;
    MPVPropertyViewer *mpvPropViewer;
};
#endif // MPVLOG_H
