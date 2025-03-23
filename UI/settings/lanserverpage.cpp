#include "lanserverpage.h"
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QNetworkInterface>
#include <QIntValidator>
#include <QFont>
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaToggleSwitch.h"
#include "globalobjects.h"
#include "LANServer/lanserver.h"

#define SETTING_KEY_SERVER_AUTOSTART "Server/AutoStart"
#define SETTING_KEY_SERVER_SYNC_PLAYTIME "Server/SyncPlayTime"
#define SETTING_KEY_SERVER_PORT "Server/Port"
#define SETTING_KEY_SERVER_AUTO_DLNA "Server/DLNAAutoStart"

LANServerPage::LANServerPage(QWidget *parent) : SettingPage(parent)
{
    SettingItemArea *serviceArea = new SettingItemArea(tr("Service"), this);

    ElaToggleSwitch *serviceSwitch = new ElaToggleSwitch(this);
    serviceSwitch->setIsToggled(GlobalObjects::lanServer->isStart());
    serviceArea->addItem(tr("Start Server"), serviceSwitch);

    ElaLineEdit *portEdit = new ElaLineEdit(this);
    QIntValidator *portValidator = new QIntValidator(this);
    portValidator->setRange(1, 65535);
    portEdit->setValidator(portValidator);
    portEdit->setText(QString::number(GlobalObjects::appSetting->value(SETTING_KEY_SERVER_PORT, 8000).toUInt()));
    portEdit->setEnabled(!GlobalObjects::lanServer->isStart());
    serviceArea->addItem(tr("Port"), portEdit);

    ElaToggleSwitch *syncUpdateTimeSwitch = new ElaToggleSwitch(this);
    syncUpdateTimeSwitch->setIsToggled(GlobalObjects::appSetting->value(SETTING_KEY_SERVER_SYNC_PLAYTIME, true).toBool());
    serviceArea->addItem(tr("Synchronous Playback Time"), syncUpdateTimeSwitch);

    ElaToggleSwitch *startDLNASwitch = new ElaToggleSwitch(this);
    startDLNASwitch->setIsToggled(GlobalObjects::appSetting->value(SETTING_KEY_SERVER_AUTO_DLNA, true).toBool());
    startDLNASwitch->setEnabled(serviceSwitch->getIsToggled());
    serviceArea->addItem(tr("Start DLNA Media Server"), startDLNASwitch);

    QLabel *addressTip = new QLabel(this);
    addressTip->setTextInteractionFlags(Qt::TextBrowserInteraction);
    addressTip->setObjectName(QStringLiteral("ServerAddressTip"));
    QStringList tips;
    tips << QString("<h2>%1</h2>").arg(tr("KikoPlay Server Address"));
    for (QHostAddress &address : QNetworkInterface::allAddresses())
    {
        if (address.protocol() == QAbstractSocket::IPv4Protocol)
        {
            tips << QString("<p style='color: rgb(220,220,220); font-size: 16px;'>%1</p>").arg(address.toString());
        }
    }
    addressTip->setText(tips.join('\n'));
    addressTip->setVisible(serviceSwitch->getIsToggled());

    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    itemVLayout->addWidget(serviceArea);
    itemVLayout->addWidget(addressTip);
    itemVLayout->addStretch(1);


    QObject::connect(serviceSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        if (checked)
        {
            quint16 port = qBound<quint16>(1, portEdit->text().toUInt(), 65535);
            portEdit->setText(QString::number(port));
            serviceSwitch->setIsToggled(GlobalObjects::lanServer->startServer(port));
            portEdit->setEnabled(!serviceSwitch->getIsToggled());
            startDLNASwitch->setEnabled(true);
            addressTip->show();
            if (serviceSwitch->getIsToggled() && startDLNASwitch->getIsToggled())
            {
                GlobalObjects::lanServer->startDLNA();
            }
        }
        else
        {
            GlobalObjects::lanServer->stopServer();
            GlobalObjects::lanServer->stopDLNA();
            portEdit->setEnabled(true);
            addressTip->hide();
            startDLNASwitch->setEnabled(false);
        }
        GlobalObjects::appSetting->setValue(SETTING_KEY_SERVER_AUTOSTART, checked);
    });

    QObject::connect(syncUpdateTimeSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::appSetting->setValue(SETTING_KEY_SERVER_SYNC_PLAYTIME, checked);
    });

    QObject::connect(startDLNASwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        if (checked)
        {
            if (serviceSwitch->getIsToggled())
            {
                GlobalObjects::lanServer->startDLNA();
            }
        }
        else
        {
            GlobalObjects::lanServer->stopDLNA();
        }
        GlobalObjects::appSetting->setValue(SETTING_KEY_SERVER_AUTO_DLNA, checked);
    });

    QObject::connect(portEdit, &QLineEdit::textChanged, this, [=](const QString &port){
        GlobalObjects::appSetting->setValue(SETTING_KEY_SERVER_PORT, port.toUInt());
    });

}

