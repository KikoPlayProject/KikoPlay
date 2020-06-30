#ifndef DOWNLOADPAGE_H
#define DOWNLOADPAGE_H
#include "settingpage.h"
class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QTextEdit;
class DownloadPage : public SettingPage
{
    Q_OBJECT
public:
    DownloadPage(QWidget *parent = nullptr);
    virtual void onAccept() override;
    virtual void onClose() override;
private:
    QLineEdit *maxDownSpeedLimit,*maxUpSpeedLimit,*seedTime,*maxConcurrent;
    QCheckBox *autoAddtoPlaylist;
    QPlainTextEdit *btTrackers;
    QTextEdit *args;

    bool downSpeedChange = false;
    bool upSpeedChange = false;
    bool btTrackerChange = false;
    bool concurrentChange = false;
    bool seedTimeChange = false;
    bool argChange = false;
};

#endif // DOWNLOADPAGE_H
