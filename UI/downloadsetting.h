#ifndef DOWNLOADSETTING_H
#define DOWNLOADSETTING_H

#include "framelessdialog.h"
class QLineEdit;
class QPlainTextEdit;
class DownloadSetting : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit DownloadSetting(QWidget *parent = nullptr);

    bool downSpeedChange;
    bool upSpeedChange;
    bool btTrackerChange;
    bool concurrentChange;
    bool seedTimeChange;
private:
    QLineEdit *maxDownSpeedLimit,*maxUpSpeedLimit,*seedTime,*maxConcurrent;
    QPlainTextEdit *btTrackers;
protected:
    virtual void onAccept();
};

#endif // DOWNLOADSETTING_H
