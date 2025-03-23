#ifndef KEYACTIONPAGE_H
#define KEYACTIONPAGE_H

#include "UI/framelessdialog.h"
#include "settingpage.h"

struct KeyActionItem;
struct KeyAction;

class KeyActionPage : public SettingPage
{
    Q_OBJECT
public:
    KeyActionPage(QWidget *parent = nullptr);
private:
    void laodInputConf(const QString &filename);
};

class KeyActionEditDialog : public CFramelessDialog
{
    Q_OBJECT
public:
    KeyActionEditDialog(const KeyActionItem *item, QWidget *parent = nullptr);
    ~KeyActionEditDialog();
public:
    QString key;
    int triggerType;
    KeyAction *action;
private:
    const KeyActionItem *refItem;
    QVector<QWidget *> actionParamWidgets;
    void resetActionWidgets(int type);

protected:
    void onAccept() override;
};

#endif // KEYACTIONPAGE_H
