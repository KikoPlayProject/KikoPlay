#ifndef SERVERSETTTING_H
#define SERVERSETTTING_H

#include "framelessdialog.h"
class QPlainTextEdit;
class ServerSettting : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit ServerSettting(QWidget *parent = nullptr);
    ~ServerSettting();
private:
    QPlainTextEdit *logInfo;
    void printLog(const QString &log);
};

#endif // SERVERSETTTING_H
