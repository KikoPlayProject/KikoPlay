#ifndef LANSERVERPAGE_H
#define LANSERVERPAGE_H
#include "settingpage.h"
class QPlainTextEdit;
class QCheckBox;
class QLineEdit;
class QTextEdit;
class LANServerPage: public SettingPage
{
    Q_OBJECT
public:
    LANServerPage(QWidget *parent = nullptr);
    virtual void onAccept() override;
    virtual void onClose() override;
private:
    QCheckBox *startServer, *syncUpdateTime;
    QLineEdit *portEdit;
    QTextEdit *logInfo;
    bool syncTimeChanged, serverStateChanged, portChanged;
    void printLog(const QString &log);
};

#endif // LANSERVERPAGE_H
