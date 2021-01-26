#ifndef SCRIPTPAGE_H
#define SCRIPTPAGE_H
#include "settingpage.h"
#include "UI/framelessdialog.h"
class ScriptBase;
class ScriptPage : public SettingPage
{
    Q_OBJECT
public:
    ScriptPage(QWidget *parent = nullptr);
    virtual void onAccept() override;
    virtual void onClose() override;
};

class ScriptSettingDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    ScriptSettingDialog(QSharedPointer<ScriptBase> script,  QWidget *parent = nullptr);
    QMap<int, QString> changedItems;

};
#endif // SCRIPTPAGE_H
