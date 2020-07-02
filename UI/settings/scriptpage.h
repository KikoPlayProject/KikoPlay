#ifndef SCRIPTPAGE_H
#define SCRIPTPAGE_H
#include "settingpage.h"

class ScriptPage : public SettingPage
{
    Q_OBJECT
public:
    ScriptPage(QWidget *parent = nullptr);
    virtual void onAccept() override;
    virtual void onClose() override;
};

#endif // SCRIPTPAGE_H
