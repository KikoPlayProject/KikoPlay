#ifndef PLAYLISTPAGE_H
#define PLAYLISTPAGE_H

#include "settingpage.h"

class PlaylistPage : public SettingPage
{
    Q_OBJECT
public:
    PlaylistPage(QWidget *parent = nullptr);

private:
    SettingItemArea *initEpMatchArea();
    SettingItemArea *initOtherArea();
};

#endif // PLAYLISTPAGE_H
