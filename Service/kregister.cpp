#include "kregister.h"
#include <QTimer>
#include <QLabel>
#include <QGridLayout>
#include "Common/notifier.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/widgets/kpushbutton.h"
#include "kservice.h"

int KRegister::sendCodeWaitTime = 0;

KRegister::KRegister(QWidget *parent) : CFramelessDialog(tr("Register"), parent, true)
{
    QLabel *emailTip = new QLabel(tr("Email"), this);
    emailEdit = new ElaLineEdit(this);

    QLabel *usernameTip=new QLabel(tr("User Name"), this);
    usernameEdit = new ElaLineEdit(this);

    QLabel *passwordTip=new QLabel(tr("Password"), this);
    passwordEdit = new ElaLineEdit(this);
    passwordEdit->setEchoMode(QLineEdit::EchoMode::Password);

    QLabel *repeatTip=new QLabel(tr("Repeat Password"), this);
    repeatEdit = new ElaLineEdit(this);
    repeatEdit->setEchoMode(QLineEdit::EchoMode::Password);

    QLabel *verificationCodeTip=new QLabel(tr("Verification Code"), this);
    codeEdit = new ElaLineEdit(this);
    QPushButton *sendCodeBtn = new KPushButton(tr("Send"), this);

    QGridLayout *epEditorGLayout = new QGridLayout(this);
    epEditorGLayout->addWidget(emailTip, 0, 0);
    epEditorGLayout->addWidget(emailEdit, 0, 1, 1, 2);
    epEditorGLayout->addWidget(usernameTip, 1, 0);
    epEditorGLayout->addWidget(usernameEdit, 1, 1, 1, 2);
    epEditorGLayout->addWidget(passwordTip, 2, 0);
    epEditorGLayout->addWidget(passwordEdit, 2, 1, 1, 2);
    epEditorGLayout->addWidget(repeatTip, 3, 0);
    epEditorGLayout->addWidget(repeatEdit, 3, 1, 1, 2);
    epEditorGLayout->addWidget(verificationCodeTip, 4, 0);
    epEditorGLayout->addWidget(codeEdit, 4, 1);
    epEditorGLayout->addWidget(sendCodeBtn, 4, 2);

    epEditorGLayout->setColumnStretch(1, 1);

    QTimer *sendTimer = new QTimer(this);
    QObject::connect(sendTimer, &QTimer::timeout, this, [=](){
        if (sendCodeWaitTime <= 0)
        {
            sendCodeWaitTime = 0;
            sendTimer->stop();
            sendCodeBtn->setEnabled(true);
            sendCodeBtn->setText(tr("Send"));
        }
        else
        {
            --sendCodeWaitTime;
            sendCodeBtn->setEnabled(false);
            sendCodeBtn->setText(QString("%1s").arg(sendCodeWaitTime));
        }
    });
    QObject::connect(sendCodeBtn, &QPushButton::clicked, this, [=](){
        const QString email = emailEdit->text().trimmed();
        if (email.isEmpty())
        {
            showMessage(tr("Email should not be empty"), NM_ERROR | NM_HIDE);
            return;
        }
        QString errInfo = KService::instance()->isValidEmail(email);
        if (!errInfo.isEmpty())
        {
            showMessage(errInfo, NM_ERROR | NM_HIDE);
            return;
        }
        if (sendTimer->isActive() || sendCodeWaitTime > 0) return;
        sendCodeWaitTime = 60;
        sendTimer->start(1000);
        sendCodeBtn->setEnabled(false);
        KService::instance()->sendVerification(email);
    });
    if (sendCodeWaitTime > 0)
    {
        sendTimer->start(1000);
        sendCodeBtn->setEnabled(false);
    }
    QObject::connect(KService::instance(), &KService::registerFinished, this, [=](int status, const QString &errMsg){
        if (status == 0)
        {
            showBusyState(false);
            showMessage(errMsg, NM_ERROR | NM_HIDE);
            return;
        }
        showMessage(tr("Register Success"), NM_HIDE);
        QTimer::singleShot(500, this, [=](){
            CFramelessDialog::onAccept();
        });
    });
}

void KRegister::onAccept()
{
    const QString email = emailEdit->text().trimmed();
    const QString username = usernameEdit->text().trimmed();
    const QString password = passwordEdit->text();
    const QString passwordRepeat = repeatEdit->text();
    const QString verificationCode = codeEdit->text().trimmed();
    if (email.isEmpty() || username.isEmpty() || password.isEmpty() || verificationCode.isEmpty())
    {
        showMessage(tr("Email/Username/Password/Verification code should not be empty"), NM_ERROR | NM_HIDE);
        return;
    }
    if (password != passwordRepeat)
    {
        showMessage(tr("The two entered passwords do not match"), NM_ERROR | NM_HIDE);
        return;
    }
    QString errInfo = KService::instance()->isValidEmail(email);
    if (!errInfo.isEmpty())
    {
        showMessage(errInfo, NM_ERROR | NM_HIDE);
        return;
    }
    errInfo = KService::instance()->isValidUserName(username);
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
    KService::instance()->registerU(email, password, username, verificationCode);
    showBusyState(true);
    showMessage(tr("Registering..."), NM_PROCESS | NM_DARKNESS_BACK);
    registeredEmail = email;
}

