#include "serversettting.h"
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QGridLayout>
#include <QCheckBox>
#include <QLabel>
#include <QSettings>
#include <QNetworkInterface>
#include <QIntValidator>
#include "globalobjects.h"
#include "LANServer/lanserver.h"
ServerSettting::ServerSettting(QWidget *parent) : CFramelessDialog(tr("LAN Server"),parent)
{
    QCheckBox *startServer=new QCheckBox(tr("Start Server"),this);
    startServer->setChecked(GlobalObjects::lanServer->isStart());

    QCheckBox *syncUpdateTime=new QCheckBox(tr("Synchronous playback time"),this);
    syncUpdateTime->setChecked(GlobalObjects::appSetting->value("Server/SyncPlayTime",true).toBool());

    QLabel *lbPort=new QLabel(tr("Port:"),this);
    QLineEdit *portEdit=new QLineEdit(this);
    QIntValidator *portValidator=new QIntValidator(this);
    portValidator->setBottom(1);
    portEdit->setValidator(portValidator);
    portEdit->setText(QString::number(GlobalObjects::appSetting->value("Server/Port",8000).toLongLong()));

    QPlainTextEdit *addressTip=new QPlainTextEdit(this);
    addressTip->setReadOnly(true);
    addressTip->appendPlainText(tr("KikoPlay Server Address:"));
    foreach (QHostAddress address, QNetworkInterface::allAddresses())
    {
       if(address.protocol() == QAbstractSocket::IPv4Protocol)
            addressTip->appendPlainText(address.toString());
    }

    logInfo=new QPlainTextEdit(this);
    logInfo->setReadOnly(true);
    logInfo->setMaximumBlockCount(256);
    for(const QString &log:GlobalObjects::lanServer->getLog())
    {
        logInfo->appendPlainText(log);
    }

    QObject::connect(startServer,&QCheckBox::clicked,[startServer,portEdit](bool checked){
        if(checked)
        {
            qint64 port=portEdit->text().toLongLong();
            if(port<=0)
            {
                port=8000;
                portEdit->setText("8000");
            }
            if(!GlobalObjects::lanServer->startServer(port).isEmpty())
            {
                startServer->setChecked(false);
                GlobalObjects::appSetting->setValue("Server/AutoStart",false);
            }
            else
            {
                portEdit->setEnabled(false);
                GlobalObjects::appSetting->setValue("Server/AutoStart",true);
            }
        }
        else
        {
            GlobalObjects::lanServer->stopServer();
            portEdit->setEnabled(true);
            GlobalObjects::appSetting->setValue("Server/AutoStart",false);
        }
    });
    QObject::connect(syncUpdateTime,&QCheckBox::clicked,[](bool checked){
       GlobalObjects::appSetting->setValue("Server/SyncPlayTime",checked);
    });
    QObject::connect(portEdit,&QLineEdit::editingFinished,[portEdit](){
       GlobalObjects::appSetting->setValue("Server/Port",portEdit->text().toLongLong());
    });
    QObject::connect(GlobalObjects::lanServer,&LANServer::showLog,this,&ServerSettting::printLog);

    QGridLayout *dialogGLayout=new QGridLayout(this);
    dialogGLayout->addWidget(startServer,0,0);
    dialogGLayout->addWidget(syncUpdateTime,0,1);
    dialogGLayout->addWidget(lbPort,1,0);
    dialogGLayout->addWidget(portEdit,1,1);
    dialogGLayout->addWidget(addressTip,2,0,1,2);
    dialogGLayout->addWidget(logInfo,3,0,1,2);
    dialogGLayout->setRowStretch(3,1);
    dialogGLayout->setColumnStretch(1,1);
    resize(400*logicalDpiX()/96, 400*logicalDpiY()/96);
}

ServerSettting::~ServerSettting()
{
    GlobalObjects::lanServer->disconnect(this);
}

void ServerSettting::printLog(const QString &log)
{
    logInfo->appendPlainText(log.trimmed());
    QTextCursor cursor = logInfo->textCursor();
    cursor.movePosition(QTextCursor::End);
    logInfo->setTextCursor(cursor);
}
