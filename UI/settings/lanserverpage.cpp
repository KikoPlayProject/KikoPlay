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
#include "globalobjects.h"
#include "LANServer/lanserver.h"

LANServerPage::LANServerPage(QWidget *parent) : SettingPage(parent), syncTimeChanged(false),
    serverStateChanged(false), portChanged(false), dlnaStateChanged(false)
{
    startServer=new QCheckBox(tr("Start Server"),this);
    startServer->setChecked(GlobalObjects::lanServer->isStart());

    syncUpdateTime=new QCheckBox(tr("Synchronous playback time"),this);
    syncUpdateTime->setChecked(GlobalObjects::appSetting->value("Server/SyncPlayTime",true).toBool());

    QLabel *lbPort=new QLabel(tr("Port:"),this);
    portEdit=new QLineEdit(this);
    QIntValidator *portValidator=new QIntValidator(this);
    portValidator->setRange(1, 65535);
    portEdit->setValidator(portValidator);
    portEdit->setText(QString::number(GlobalObjects::appSetting->value("Server/Port",8000).toUInt()));
    portEdit->setEnabled(!GlobalObjects::lanServer->isStart());

    startDLNA = new QCheckBox(tr("Start DLNA Media Server"), this);
    startDLNA->setChecked(GlobalObjects::appSetting->value("Server/DLNAAutoStart",false).toBool());
    startDLNA->setEnabled(startServer->isChecked());

    QPlainTextEdit *addressTip=new QPlainTextEdit(this);
    addressTip->setReadOnly(true);
    addressTip->appendPlainText(tr("KikoPlay Server Address:"));
    addressTip->setFont(QFont(GlobalObjects::normalFont, 14));
    for(QHostAddress address : QNetworkInterface::allAddresses())
    {
       if(address.protocol() == QAbstractSocket::IPv4Protocol)
            addressTip->appendPlainText(address.toString());
    }

    QObject::connect(startServer,&QCheckBox::clicked,[this](bool checked){
        serverStateChanged = true;
        if(checked)
        {
            quint16 port=qBound<quint16>(1,portEdit->text().toUInt(), 65535);
            portEdit->setText(QString::number(port));
            startServer->setChecked(GlobalObjects::lanServer->startServer(port));
            portEdit->setEnabled(!startServer->isChecked());
            startDLNA->setEnabled(true);
            if(startServer->isChecked() && startDLNA->isChecked())
            {
                GlobalObjects::lanServer->startDLNA();
            }
        }
        else
        {
            GlobalObjects::lanServer->stopServer();
            GlobalObjects::lanServer->stopDLNA();
            portEdit->setEnabled(true);
            startDLNA->setEnabled(false);
        }
    });
    QObject::connect(syncUpdateTime,&QCheckBox::stateChanged,[this](bool ){
       syncTimeChanged = true;
    });
    QObject::connect(portEdit, &QLineEdit::textChanged, this, [this](){
        portChanged = true;
    });
    QObject::connect(startDLNA,&QCheckBox::stateChanged,[this](bool checked){
       dlnaStateChanged = true;
       if(checked)
       {
           if(startServer->isChecked())
           {
               GlobalObjects::lanServer->startDLNA();
           }
       }
       else
       {
           GlobalObjects::lanServer->stopDLNA();
       }
    });

    QGridLayout *pGLayout=new QGridLayout(this);
    pGLayout->setContentsMargins(0, 0, 0, 0);
    pGLayout->addWidget(startServer, 0, 0);
    pGLayout->addWidget(syncUpdateTime, 0, 1);
    pGLayout->addWidget(lbPort, 1, 0);
    pGLayout->addWidget(portEdit,1, 1);
    pGLayout->addWidget(startDLNA, 2, 0, 1, 2);
    pGLayout->addWidget(addressTip, 3, 0, 1, 2);
    pGLayout->setRowStretch(3, 1);
    pGLayout->setColumnStretch(1, 1);
}

void LANServerPage::onAccept()
{
    if(serverStateChanged) GlobalObjects::appSetting->setValue("Server/AutoStart",startServer->isChecked());
    if(syncTimeChanged) GlobalObjects::appSetting->setValue("Server/SyncPlayTime",syncUpdateTime->isChecked());
    if(portChanged) GlobalObjects::appSetting->setValue("Server/Port",portEdit->text().toUInt());
    if(dlnaStateChanged) GlobalObjects::appSetting->setValue("Server/DLNAAutoStart",startDLNA->isChecked());
}

void LANServerPage::onClose()
{
    if(serverStateChanged)
    {
        GlobalObjects::appSetting->setValue("Server/AutoStart",startServer->isChecked());
        if(syncTimeChanged) GlobalObjects::appSetting->setValue("Server/SyncPlayTime",syncUpdateTime->isChecked());
        if(portChanged) GlobalObjects::appSetting->setValue("Server/Port",portEdit->text().toUInt());
    }
    if(dlnaStateChanged)
    {
        GlobalObjects::appSetting->setValue("Server/DLNAAutoStart",startDLNA->isChecked());
    }
}

