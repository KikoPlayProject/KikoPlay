#ifndef PLAYERPAGE_H
#define PLAYERPAGE_H
#include "settingpage.h"
#include <QVector>
class QPlainTextEdit;
class QTabWidget;
class PlayerPage : public SettingPage
{
    Q_OBJECT
public:
    PlayerPage(QWidget *parent = nullptr);

private:
    SettingItemArea *initBehaviorArea();
    SettingItemArea *initMpvArea();
    SettingItemArea *initSubArea();
};
#endif // PLAYERPAGE_H
