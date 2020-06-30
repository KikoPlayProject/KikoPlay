#include "downloadpage.h"
#include <QPlainTextEdit>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QSettings>
#include <QGridLayout>
#include <QCheckBox>
#include <QIntValidator>
#include <QSyntaxHighlighter>
#include "globalobjects.h"
namespace
{
    class OptionHighLighter : public QSyntaxHighlighter
    {
    public:
        OptionHighLighter(QTextDocument *parent):QSyntaxHighlighter(parent)
        {
            optionNameFormat.setForeground(QColor(41,133,199));
            optionContentFormat.setForeground(QColor(245,135,31));
            commentFormat.setForeground(QColor(142,142,142));
            optionRe.setPattern("\\s*([^#=]+)=?(.*)\n?");
            commentRe.setPattern("\\s*#.*\n?");
        }
        // QSyntaxHighlighter interface
    protected:
        virtual void highlightBlock(const QString &text)
        {
            int index= commentRe.indexIn(text);
            if(index!=-1)
            {
                setFormat(index, commentRe.matchedLength(), commentFormat);
                return;
            }
            index = optionRe.indexIn(text);
            if (index>-1)
            {
                QStringList opList = optionRe.capturedTexts();
                int opNamePos = optionRe.pos(1), opContentPos = optionRe.pos(2);
                if (opNamePos>-1)
                {
                    setFormat(opNamePos, opList.at(1).length(), optionNameFormat);
                }
                if (opContentPos>-1)
                {
                    setFormat(opContentPos + 1, opList.at(2).length() - 1, optionContentFormat);
                }
            }
        }
    private:
        QTextCharFormat optionNameFormat;
        QTextCharFormat optionContentFormat;
        QTextCharFormat commentFormat;
        QRegExp optionRe,commentRe;
    };
}


DownloadPage::DownloadPage(QWidget *parent) : SettingPage(parent)
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

    autoAddtoPlaylist=new QCheckBox(tr("Automatically add to playlist after download"),this);
    autoAddtoPlaylist->setChecked(GlobalObjects::appSetting->value("Download/AutoAddToList",false).toBool());
    QObject::connect(autoAddtoPlaylist,&QCheckBox::stateChanged,[](int state){
        GlobalObjects::appSetting->setValue("Download/AutoAddToList",state==Qt::Checked);
    });

    QLabel *btTrackerLabel=new QLabel(tr("Extra BT Trackers: "),this);
    btTrackers=new QPlainTextEdit(this);
    btTrackers->appendPlainText(GlobalObjects::appSetting->value("Download/Trackers",QStringList()).toStringList().join('\n'));
    QObject::connect(btTrackers,&QPlainTextEdit::textChanged,[this](){
       btTrackerChange=true;
    });

    QLabel *startupArgsLabel=new QLabel(tr("Aria2 Startup Args: "), this);
    args = new QTextEdit(this);
    args->setFont(QFont("Consolas",10));
    new OptionHighLighter(args->document());
    args->setPlainText(GlobalObjects::appSetting->value("Download/Aria2Args","").toString());
    QObject::connect(args,&QTextEdit::textChanged,[this](){
       argChange=true;
    });

    QGridLayout *settingGLayout=new QGridLayout(this);
    settingGLayout->setContentsMargins(0, 0, 0, 0);
    settingGLayout->addWidget(maxDownSpeedLabel,0,0);
    settingGLayout->addWidget(maxDownSpeedLimit,0,1);
    settingGLayout->addWidget(maxUpSpeedLabel,1,0);
    settingGLayout->addWidget(maxUpSpeedLimit,1,1);
    settingGLayout->addWidget(seedTimeLabel,2,0);
    settingGLayout->addWidget(seedTime,2,1);
    settingGLayout->addWidget(maxConcurrentLabel,3,0);
    settingGLayout->addWidget(maxConcurrent,3,1);
    settingGLayout->addWidget(autoAddtoPlaylist,4,0,1,2);
    settingGLayout->addWidget(btTrackerLabel,5,0);
    settingGLayout->addWidget(btTrackers,6,0,1,2);
    settingGLayout->addWidget(startupArgsLabel,7,0);
    settingGLayout->addWidget(args,8,0,1,2);
    settingGLayout->setRowStretch(6,1);
    settingGLayout->setRowStretch(8,1);
    settingGLayout->setColumnStretch(1,1);

}

void DownloadPage::onClose()
{
    if(downSpeedChange)
    {
        int mdSpeed = maxDownSpeedLimit->text().toInt();
        changedValues["downSpeed"] = mdSpeed;
        GlobalObjects::appSetting->setValue("Download/MaxDownloadLimit", mdSpeed);
    }
    if(upSpeedChange)
    {
        int muSpeed = maxUpSpeedLimit->text().toInt();
        changedValues["upSpeed"] = muSpeed;
        GlobalObjects::appSetting->setValue("Download/MaxUploadLimit", muSpeed);
    }
    if(seedTimeChange)
    {
        int sTime = seedTime->text().toInt();
        changedValues["seedTime"] = sTime;
        GlobalObjects::appSetting->setValue("Download/SeedTime", sTime);
    }
    if(concurrentChange)
    {
        int mConcurrent = maxConcurrent->text().toInt();
        changedValues["concurrent"] = mConcurrent;
        GlobalObjects::appSetting->setValue("Download/ConcurrentDownloads", mConcurrent);
    }
    if(btTrackerChange)
    {
        QStringList trackers = btTrackers->toPlainText().split('\n',QString::SkipEmptyParts);
        changedValues["btTracker"] = trackers;
        GlobalObjects::appSetting->setValue("Download/Trackers", trackers);
    }
    if(argChange)
    {
        GlobalObjects::appSetting->setValue("Download/Aria2Args",args->toPlainText());
    }
}
