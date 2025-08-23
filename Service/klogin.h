#ifndef KLOGIN_H
#define KLOGIN_H

#include "UI/framelessdialog.h"

class ElaLineEdit;
class ElaCheckBox;
class KLogin : public CFramelessDialog
{
    Q_OBJECT
public:
    KLogin(QWidget *parent = nullptr);

protected:
    virtual void onAccept();

private:
    ElaLineEdit *emailEdit, *passwordEdit;
    ElaCheckBox *rememberPasswordCheck;
};

#endif // KLOGIN_H
