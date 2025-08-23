#ifndef PLAYLISTPAGE_H
#define PLAYLISTPAGE_H

#include "settingpage.h"
#include "../framelessdialog.h"

class PlaylistPage : public SettingPage
{
    Q_OBJECT
public:
    PlaylistPage(QWidget *parent = nullptr);

private:
    SettingItemArea *initEpMatchArea();
    SettingItemArea *initOtherArea();
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
#endif // PLAYLISTPAGE_H
