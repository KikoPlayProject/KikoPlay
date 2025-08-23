#ifndef KREGISTER_H
#define KREGISTER_H

#include "UI/framelessdialog.h"

class ElaLineEdit;
class KRegister : public CFramelessDialog
{
    Q_OBJECT
public:
    KRegister(QWidget *parent = nullptr);

    QString registeredEmail;

protected:
    virtual void onAccept();

private:
    ElaLineEdit *emailEdit, *usernameEdit, *passwordEdit, *repeatEdit, *codeEdit;
    static int sendCodeWaitTime;  // s

};

#endif // KREGISTER_H
