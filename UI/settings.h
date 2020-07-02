#ifndef SETTINGS_H
#define SETTINGS_H
#include "framelessdialog.h"
#include "settings/settingpage.h"
class QStackedLayout;
class Settings : public CFramelessDialog
{
    Q_OBJECT
public:
    enum Page
    {
        PAGE_UI,
        PAGE_MPV,
        PAGE_DOWN,
        PAGE_LAN,
        PAGE_SCRIPT,
        PAGE_STOP
    };
    Settings(Page page=PAGE_UI, QWidget *parent = nullptr);
    const SettingPage *getPage(Page page) const {return pages[page];}
protected:
    void onAccept() override;
    void onClose() override;
private:
    QHash<Page, SettingPage *> pages;
    QStackedLayout *pageSLayout;
    SettingPage *getOrCreatePage(Page p);
};

#endif // SETTINGS_H
