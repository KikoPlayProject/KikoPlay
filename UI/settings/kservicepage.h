#ifndef KSERVICEPAGE_H
#define KSERVICEPAGE_H

#include "settingpage.h"
#include "../framelessdialog.h"

class KServicePage : public SettingPage
{
    Q_OBJECT
public:
    KServicePage(QWidget *parent = nullptr);

private:
#ifdef KSERVICE
    SettingItemArea *initAccountArea();
    SettingItemArea *initInfoRetrievalArea();
#endif
    SettingItemArea *initEpMatchArea();
};

#ifdef KSERVICE
class QListWidget;
class KLibraryOrderDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    KLibraryOrderDialog(QWidget *parent = nullptr);
private:
    QListWidget *sourecOrderView;
protected:
    void onAccept() override;
};
#endif
#endif // KSERVICEPAGE_H
