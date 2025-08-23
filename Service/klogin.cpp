#include "klogin.h"
#include <QLabel>
#include <QGridLayout>
#include "Common/notifier.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/widgets/kpushbutton.h"
#include "kservice.h"
#include "kregister.h"
#include "globalobjects.h"

#define SETTING_KEY_REMEMBER_PW "Profile/RememberPassword"
#define SETTING_KEY_LAST_EMAIL "Profile/LastEmail"
#define SETTING_KEY_LAST_PASSWORD "Profile/lastPassword"


KLogin::KLogin(QWidget *parent) : CFramelessDialog(tr("Login"), parent, true)
{
    QLabel *emailTip = new QLabel(tr("Email"), this);
    emailEdit = new ElaLineEdit(this);
    emailEdit->setText(GlobalObjects::appSetting->value(SETTING_KEY_LAST_EMAIL).toString());

    QLabel *passwordTip=new QLabel(tr("Password"), this);
    passwordEdit = new ElaLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::EchoMode::Password);

    QPushButton *registerBtn = new KPushButton(tr("Register"), this);
    registerBtn->setAutoDefault(false);
    rememberPasswordCheck = new ElaCheckBox(tr("Remember"), this);
    rememberPasswordCheck->setChecked(GlobalObjects::appSetting->value(SETTING_KEY_REMEMBER_PW, true).toBool());
    if (rememberPasswordCheck->isChecked())
    {
        QString pw = QByteArray::fromBase64(GlobalObjects::appSetting->value(SETTING_KEY_LAST_PASSWORD).toByteArray());
        passwordEdit->setText(pw);
    }

    QGridLayout *epEditorGLayout = new QGridLayout(this);
    epEditorGLayout->addWidget(emailTip, 0, 0);
    epEditorGLayout->addWidget(emailEdit, 0, 1);
    epEditorGLayout->addWidget(registerBtn, 0, 2);
    epEditorGLayout->addWidget(passwordTip, 1, 0);
    epEditorGLayout->addWidget(passwordEdit, 1, 1);
    epEditorGLayout->addWidget(rememberPasswordCheck, 1, 2);

    epEditorGLayout->setColumnStretch(1, 1);

    QObject::connect(rememberPasswordCheck, &QCheckBox::stateChanged, this, [=](int state){
        GlobalObjects::appSetting->value(SETTING_KEY_REMEMBER_PW, state == Qt::CheckState::Checked);
    });
    QObject::connect(registerBtn, &QPushButton::clicked, this, [=](){
        KRegister reg(this);
        if (QDialog::Accepted == reg.exec())
        {
            emailEdit->setText(reg.registeredEmail);
        }
    });
    QObject::connect(passwordEdit, &QLineEdit::returnPressed, this, &KLogin::onAccept);

    QObject::connect(KService::instance(), &KService::loginFinished, this, [=](int status, const QString &errMsg){
        if (status == 0)
        {
            showBusyState(false);
            showMessage(errMsg, NM_ERROR | NM_HIDE);
            return;
        }
        showMessage(tr("Login Success"), NM_HIDE);
        GlobalObjects::appSetting->setValue(SETTING_KEY_LAST_EMAIL, emailEdit->text().trimmed());
        GlobalObjects::appSetting->setValue(SETTING_KEY_LAST_PASSWORD, rememberPasswordCheck->isChecked() ? passwordEdit->text().toUtf8().toBase64() : QByteArray());
        QTimer::singleShot(500, this, [=](){
            CFramelessDialog::onAccept();
        });
    });
}

void KLogin::onAccept()
{
    const QString email = emailEdit->text().trimmed();
    const QString password = passwordEdit->text();
    if (email.isEmpty() || password.isEmpty())
    {
        showMessage(tr("Email/Password should not be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    QString errInfo = KService::instance()->isValidEmail(email);
    if (!errInfo.isEmpty())
    {
        showMessage(errInfo, NM_ERROR | NM_HIDE);
        return;
    }
    errInfo = KService::instance()->isValidPassword(password);
    if (!errInfo.isEmpty())
    {
        showMessage(errInfo, NM_ERROR | NM_HIDE);
        return;
    }
    KService::instance()->login(email, password);
    showBusyState(true);
    showMessage(tr("Logging in..."), NM_PROCESS | NM_DARKNESS_BACK);
}
