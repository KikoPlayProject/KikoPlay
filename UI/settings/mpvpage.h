#ifndef MPVPAGE_H
#define MPVPAGE_H
#include "settingpage.h"
class QTextEdit;
class MPVPage : public SettingPage
{
    Q_OBJECT
public:
    MPVPage(QWidget *parent = nullptr);
    virtual void onClose();
private:
    QTextEdit *parameterEdit;
    bool paramChange = false;
};

#endif // MPVPAGE_H
