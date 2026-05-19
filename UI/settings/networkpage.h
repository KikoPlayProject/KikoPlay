#ifndef NETWORKPAGE_H
#define NETWORKPAGE_H
#include "settingpage.h"
class QVBoxLayout;
class NetworkPage : public SettingPage
{
    Q_OBJECT
public:
    NetworkPage(QWidget *parent = nullptr);
private:
    void initProxyArea(QVBoxLayout *vLayout);
    void initLANServerArea(QVBoxLayout *vLayout);
};

#endif // NETWORKPAGE_H
