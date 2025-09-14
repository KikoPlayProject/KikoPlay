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
        PAGE_GENERAL,
        PAGE_DANMU,
        PAGE_PLAYER,
        PAGE_PLAYLIST,
        PAGE_KEYACTION,
        PAGE_DOWN,
        PAGE_LAN,
        PAGE_SCRIPT,
        PAGE_APP,
        PAGE_STOP
    };
    Settings(Page page=PAGE_GENERAL, QWidget *parent = nullptr);
    const SettingPage *getPage(Page page) const {return pages[page];}
protected:
    void onClose() override;
private:
    SettingPage *pages[PAGE_STOP];
    QStackedLayout *pageSLayout;
    SettingPage *getOrCreatePage(Page p);
};

#endif // SETTINGS_H
