#ifndef DOWNLOADPAGE_H
#define DOWNLOADPAGE_H
#include "settingpage.h"
#include "../framelessdialog.h"
class QPlainTextEdit;
class QTextEdit;
class DownloadPage : public SettingPage
{
    Q_OBJECT
public:
    DownloadPage(QWidget *parent = nullptr);
};

class TrackerEditDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    TrackerEditDialog(QWidget *parent = nullptr);
    bool manualTrackerListChanged = false;
private:
    QPlainTextEdit *btTrackers{nullptr};
protected:
    void onAccept() override;
};

class Aria2OptionEditDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    Aria2OptionEditDialog(QWidget *parent = nullptr);
    bool argChanged = false;
private:
    QTextEdit *argEdit{nullptr};
protected:
    void onAccept() override;
};


#endif // DOWNLOADPAGE_H
