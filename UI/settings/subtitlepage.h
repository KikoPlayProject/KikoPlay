#ifndef SUBTITLEPAGE_H
#define SUBTITLEPAGE_H

#include "settingpage.h"

class SubtitlePage : public SettingPage
{
    Q_OBJECT
public:
    SubtitlePage(QWidget *parent = nullptr);

    virtual void onAccept();
    virtual void onClose();
};

#endif // SUBTITLEPAGE_H
