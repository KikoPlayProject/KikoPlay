#ifndef DOWNLOADSETTING_H
#define DOWNLOADSETTING_H

#include "framelessdialog.h"
class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
class QTextEdit;
class DownloadSetting : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit DownloadSetting(QWidget *parent = nullptr);

    bool downSpeedChange = false;
    bool upSpeedChange = false;
    bool btTrackerChange = false;
    bool concurrentChange = false;
    bool seedTimeChange = false;
    bool argChange = false;
private:
    QLineEdit *maxDownSpeedLimit,*maxUpSpeedLimit,*seedTime,*maxConcurrent;
    QCheckBox *autoAddtoPlaylist;
    QPlainTextEdit *btTrackers;
    QTextEdit *args;
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // DOWNLOADSETTING_H
