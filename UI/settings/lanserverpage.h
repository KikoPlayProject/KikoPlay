#ifndef LANSERVERPAGE_H
#define LANSERVERPAGE_H
#include "settingpage.h"
class QPlainTextEdit;
class QCheckBox;
class QLineEdit;
class LANServerPage: public SettingPage
{
    Q_OBJECT
public:
    LANServerPage(QWidget *parent = nullptr);
    virtual void onClose() override;
private:
    QCheckBox *startServer, *syncUpdateTime;
    QLineEdit *portEdit;
    QPlainTextEdit *logInfo;
    bool syncTimeChanged, serverStateChanged, portChanged;
    void printLog(const QString &log);
};

#endif // LANSERVERPAGE_H
