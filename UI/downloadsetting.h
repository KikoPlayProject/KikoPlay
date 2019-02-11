#ifndef DOWNLOADSETTING_H
#define DOWNLOADSETTING_H

#include "framelessdialog.h"
class QLineEdit;
class QPlainTextEdit;
class QCheckBox;
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
    QCheckBox *autoAddtoPlaylist;
    QPlainTextEdit *btTrackers;
protected:
    virtual void onAccept();
    virtual void onClose();
};

#endif // DOWNLOADSETTING_H
