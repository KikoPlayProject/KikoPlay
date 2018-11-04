#include "downloadsetting.h"
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QLabel>
#include <QSettings>
#include <QGridLayout>
#include <QIntValidator>
#include "globalobjects.h"
DownloadSetting::DownloadSetting(QWidget *parent) : CFramelessDialog(tr("Download Setting"),parent,true,true,false)
{
    QLabel *maxDownSpeedLabel=new QLabel(tr("Max Download Limit(KB): "),this);
    maxDownSpeedLimit=new QLineEdit(this);
    maxDownSpeedLimit->setText(QString::number(GlobalObjects::appSetting->value("Download/MaxDownloadLimit",0).toInt()));
    QObject::connect(maxDownSpeedLimit,&QLineEdit::textChanged,[this](){
       downSpeedChange=true;
    });

    QLabel *maxUpSpeedLabel=new QLabel(tr("Max Uploas Limit(KB): "),this);
    maxUpSpeedLimit=new QLineEdit(this);
    maxUpSpeedLimit->setText(QString::number(GlobalObjects::appSetting->value("Download/MaxUploadLimit",0).toInt()));
    QObject::connect(maxUpSpeedLimit,&QLineEdit::textChanged,[this](){
       upSpeedChange=true;
    });

    QLabel *seedTimeLabel=new QLabel(tr("Seed Time(min): "),this);
    seedTime=new QLineEdit(this);
    seedTime->setText(QString::number(GlobalObjects::appSetting->value("Download/SeedTime",5).toInt()));
    QObject::connect(seedTime,&QLineEdit::textChanged,[this](){
       seedTimeChange=true;
    });

    QLabel *maxConcurrentLabel=new QLabel(tr("Max Concurrent Downloads: "),this);
    maxConcurrent=new QLineEdit(this);
    maxConcurrent->setText(QString::number(GlobalObjects::appSetting->value("Download/ConcurrentDownloads",5).toInt()));
    QObject::connect(maxConcurrent,&QLineEdit::textChanged,[this](){
       concurrentChange=true;
    });

    QLabel *btTrackerLabel=new QLabel(tr("Extra BT Trackers: "),this);
    btTrackers=new QPlainTextEdit(this);
    btTrackers->appendPlainText(GlobalObjects::appSetting->value("Download/Trackers",QStringList()).toStringList().join('\n'));
    QObject::connect(btTrackers,&QPlainTextEdit::textChanged,[this](){
       btTrackerChange=true;
    });

    QIntValidator *speedAndTimeValidator=new QIntValidator(this);
    speedAndTimeValidator->setBottom(0);
    maxDownSpeedLimit->setValidator(speedAndTimeValidator);
    maxUpSpeedLimit->setValidator(speedAndTimeValidator);
    seedTime->setValidator(speedAndTimeValidator);
    QIntValidator *concurrentValidator=new QIntValidator(1,10,this);
    maxConcurrent->setValidator(concurrentValidator);

    QGridLayout *settingGLayout=new QGridLayout(this);
    settingGLayout->addWidget(maxDownSpeedLabel,0,0);
    settingGLayout->addWidget(maxDownSpeedLimit,0,1);
    settingGLayout->addWidget(maxUpSpeedLabel,1,0);
    settingGLayout->addWidget(maxUpSpeedLimit,1,1);
    settingGLayout->addWidget(seedTimeLabel,2,0);
    settingGLayout->addWidget(seedTime,2,1);
    settingGLayout->addWidget(maxConcurrentLabel,3,0);
    settingGLayout->addWidget(maxConcurrent,3,1);
    settingGLayout->addWidget(btTrackerLabel,4,0);
    settingGLayout->addWidget(btTrackers,5,0,1,2);
    settingGLayout->setRowStretch(5,1);
    settingGLayout->setColumnStretch(1,1);
}

void DownloadSetting::onAccept()
{
    if(downSpeedChange)GlobalObjects::appSetting->setValue("Download/MaxDownloadLimit",maxDownSpeedLimit->text().toInt());
    if(upSpeedChange)GlobalObjects::appSetting->setValue("Download/MaxUploadLimit",maxUpSpeedLimit->text().toInt());
    if(seedTimeChange)GlobalObjects::appSetting->setValue("Download/SeedTime",seedTime->text().toInt());
    if(concurrentChange)GlobalObjects::appSetting->setValue("Download/ConcurrentDownloads",maxConcurrent->text().toInt());
    if(btTrackerChange)GlobalObjects::appSetting->setValue("Download/Trackers",btTrackers->toPlainText().split('\n',QString::SkipEmptyParts));
    CFramelessDialog::onAccept();
}
