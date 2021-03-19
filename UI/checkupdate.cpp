#include "checkupdate.h"
#include <QLabel>
#include <QVBoxLayout>
#include <QFile>
#include <QIODevice>
#include "Common/network.h"
#include "globalobjects.h"
CheckUpdate::CheckUpdate(QWidget *parent) : CFramelessDialog(tr("Check For Updates"),parent)
{
    QLabel *curVersionLabel=new QLabel(this);
    QLabel *newVersionLabel=new QLabel(this);
    QVBoxLayout *versionVLayout=new QVBoxLayout(this);
    versionVLayout->addWidget(curVersionLabel);
    versionVLayout->addWidget(newVersionLabel);
    setResizeable(false);

    QFile version(":/res/version.json");
    version.open(QIODevice::ReadOnly);
    QJsonObject curVersionObj = QJsonDocument::fromJson(version.readAll()).object();
    QString versionStr=curVersionObj.value("Version").toString();
    curVersionLabel->setText(tr("Current Version: %1").arg(versionStr));

    newVersionLabel->setText(tr("Checking for updates..."));
    showBusyState(true);
    QTimer::singleShot(500,[this,newVersionLabel,versionStr](){
        try
        {
            QString updatePath(GlobalObjects::appSetting->value("KikoPlay/checkUpdateURL", "https://raw.githubusercontent.com/KikoPlayProject/KikoPlay/master/newVersion/version.json").toString());
            Network::Reply reply(Network::httpGet(updatePath,QUrlQuery()));
            if(reply.hasError) throw Network::NetworkError(reply.errInfo);
            QJsonObject newVersionObj(Network::toJson(reply.content).object());
            QString nVersionStr=newVersionObj.value("Version").toString();
            QString downloadURL=newVersionObj.value("URL").toString();
            QStringList curVer(versionStr.split('.',QString::SkipEmptyParts)),newVer(nVersionStr.split('.',QString::SkipEmptyParts));
            int i=0;
            bool hasNewversion=false;
            for(;i<3;++i)
            {
                int cv=curVer.at(i).toInt(),nv=newVer.at(i).toInt();
                if(cv<nv)
                {
                    hasNewversion=true;
                    break;
                }
                else if(cv>nv)
                {
                    hasNewversion=false;
                    break;
                }
            }
            if(hasNewversion)
            {
                newVersionLabel->setText(tr("Find new Version: %1  <a href=\"%2\">Click To Download</a>").arg(nVersionStr).arg(downloadURL));
				newVersionLabel->setOpenExternalLinks(true);
            }
            else
            {
                newVersionLabel->setText(tr("Already Newast"));
            }
        }
        catch(Network::NetworkError &error)
        {
            newVersionLabel->setText(error.errorInfo);
        }
        showBusyState(false);
    });
}
