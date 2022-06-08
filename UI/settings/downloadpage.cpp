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
#include <QPushButton>
#include <QTreeView>
#include <QAction>
#include <QHeaderView>
#include "Download/trackersubscriber.h"
#include "../inputdialog.h"
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
    QIntValidator *intValidator=new QIntValidator(this);
    intValidator->setBottom(0);

    QLabel *maxDownSpeedLabel=new QLabel(tr("Max Download Limit(KB): "),this);
    maxDownSpeedLimit=new QLineEdit(this);
    maxDownSpeedLimit->setValidator(intValidator);
    maxDownSpeedLimit->setText(QString::number(GlobalObjects::appSetting->value("Download/MaxDownloadLimit",0).toInt()));
    QObject::connect(maxDownSpeedLimit,&QLineEdit::textChanged,[this](){
       downSpeedChange=true;
    });

    QLabel *maxUpSpeedLabel=new QLabel(tr("Max Uploas Limit(KB): "),this);
    maxUpSpeedLimit=new QLineEdit(this);
    maxUpSpeedLimit->setValidator(intValidator);
    maxUpSpeedLimit->setText(QString::number(GlobalObjects::appSetting->value("Download/MaxUploadLimit",0).toInt()));
    QObject::connect(maxUpSpeedLimit,&QLineEdit::textChanged,[this](){
       upSpeedChange=true;
    });

    QLabel *seedTimeLabel=new QLabel(tr("Seed Time(min): "),this);
    seedTime=new QLineEdit(this);
    seedTime->setValidator(intValidator);
    seedTime->setText(QString::number(GlobalObjects::appSetting->value("Download/SeedTime",5).toInt()));
    QObject::connect(seedTime,&QLineEdit::textChanged,[this](){
       seedTimeChange=true;
    });

    QLabel *maxConcurrentLabel=new QLabel(tr("Max Concurrent Downloads: "),this);
    maxConcurrent=new QLineEdit(this);
    maxConcurrent->setValidator(intValidator);
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
    QPushButton *trackerSubscribe = new QPushButton(tr("Tracker Subscribe"), this);
    QObject::connect(trackerSubscribe, &QPushButton::clicked, this, [=](){
        TrackerSubscribeDialog dialog(this);
        dialog.exec();
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
    settingGLayout->addWidget(maxDownSpeedLimit,0,1,1,2);
    settingGLayout->addWidget(maxUpSpeedLabel,1,0);
    settingGLayout->addWidget(maxUpSpeedLimit,1,1,1,2);
    settingGLayout->addWidget(seedTimeLabel,2,0);
    settingGLayout->addWidget(seedTime,2,1,1,2);
    settingGLayout->addWidget(maxConcurrentLabel,3,0);
    settingGLayout->addWidget(maxConcurrent,3,1,1,2);
    settingGLayout->addWidget(autoAddtoPlaylist,4,0,1,3);
    settingGLayout->addWidget(btTrackerLabel,5,0);
    settingGLayout->addWidget(trackerSubscribe, 5, 2);
    settingGLayout->addWidget(btTrackers,6,0,1,3);
    settingGLayout->addWidget(startupArgsLabel,7,0);
    settingGLayout->addWidget(args,8,0,1,3);
    settingGLayout->setRowStretch(6,1);
    settingGLayout->setRowStretch(8,1);
    settingGLayout->setColumnStretch(1,1);

}

void DownloadPage::onAccept()
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
        QStringList trackers = btTrackers->toPlainText().split('\n', Qt::SkipEmptyParts);
        changedValues["btTracker"] = trackers;
        GlobalObjects::appSetting->setValue("Download/Trackers", trackers);
    }
    if(argChange)
    {
        GlobalObjects::appSetting->setValue("Download/Aria2Args",args->toPlainText());
    }
}

void DownloadPage::onClose()
{

}

TrackerSubscribeDialog::TrackerSubscribeDialog(QWidget *parent) :
     CFramelessDialog(tr("Tracker Subscribe"), parent)
{
    QPushButton *addTrackerSource = new QPushButton(tr("Add"), this);
    QPushButton *checkAll = new QPushButton(tr("Check All"), this);
    QCheckBox *autoCheck = new QCheckBox(tr("Auto Check"), this);
    QTreeView *trackerSrcView = new QTreeView(this);
    trackerSrcView->setRootIsDecorated(false);
    trackerSrcView->setAlternatingRowColors(true);
    QGridLayout *tsGLayout = new QGridLayout(this);
    QPlainTextEdit *trackerText = new QPlainTextEdit(this);
    trackerText->setReadOnly(true);
    tsGLayout->addWidget(addTrackerSource, 0, 0);
    tsGLayout->addWidget(autoCheck, 0, 2);
    tsGLayout->addWidget(checkAll, 0, 3);
    tsGLayout->addWidget(trackerSrcView, 1, 0, 1, 4);
    tsGLayout->addWidget(trackerText, 2, 0, 1, 4);
    tsGLayout->setRowStretch(1, 1);
    tsGLayout->setRowStretch(2, 1);
    tsGLayout->setColumnStretch(1, 1);
    tsGLayout->setContentsMargins(0, 0, 0, 0);

    QObject::connect(addTrackerSource, &QPushButton::clicked, this, [=](){
        LineInputDialog input(tr("Subscirbe"), tr("URL"), "", "DialogSize/AddTrackerSubscribe", false, this);
        if(QDialog::Accepted == input.exec())
        {
            TrackerSubscriber::subscriber()->add(input.text);
        }
    });
    QObject::connect(checkAll, &QPushButton::clicked, this, [=](){
        TrackerSubscriber::subscriber()->check(-1);
    });
    QObject::connect(autoCheck,&QCheckBox::stateChanged,[](int state){
        TrackerSubscriber::subscriber()->setAutoCheck(state == Qt::CheckState::Checked);
    });
    autoCheck->setChecked(GlobalObjects::appSetting->value("Download/TrackerSubscriberAutoCheck", true).toBool());

    trackerSrcView->setModel(TrackerSubscriber::subscriber());
    trackerSrcView->setContextMenuPolicy(Qt::ActionsContextMenu);
    trackerSrcView->setSelectionMode(QAbstractItemView::SingleSelection);
    QAction *actRemoveSubscirbe = new QAction(tr("Remove Subscribe"), trackerSrcView);
    QObject::connect(actRemoveSubscirbe, &QAction::triggered, this, [=](){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        TrackerSubscriber::subscriber()->remove(selection.first().row());
    });
    QAction *actCheck = new QAction(tr("Check"), trackerSrcView);
    QObject::connect(actCheck, &QAction::triggered, this, [=](){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if(selection.size()==0) return;
        TrackerSubscriber::subscriber()->check(selection.first().row());
    });
    trackerSrcView->addAction(actCheck);
    trackerSrcView->addAction(actRemoveSubscirbe);
    QObject::connect(trackerSrcView->selectionModel(), &QItemSelectionModel::selectionChanged,this, [=](){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if(selection.size()==0)
        {
            trackerText->clear();
        }
        else
        {
            QStringList trackers = TrackerSubscriber::subscriber()->getTrackers(selection.first().row());
            trackerText->setPlainText(trackers.join('\n'));
        }

    });
    QObject::connect(TrackerSubscriber::subscriber(), &TrackerSubscriber::checkStateChanged, this, [=](bool checking){
        addTrackerSource->setEnabled(!checking);
        checkAll->setEnabled(!checking);
        autoCheck->setEnabled(!checking);
        trackerSrcView->setEnabled(!checking);
    });
    QObject::connect(TrackerSubscriber::subscriber(), &TrackerSubscriber::trackerListChanged, this, [=](int index){
        auto selection = trackerSrcView->selectionModel()->selectedRows();
        if(selection.size()==0 || selection.first().row()!=index) return;
        QStringList trackers = TrackerSubscriber::subscriber()->getTrackers(selection.first().row());
        trackerText->setPlainText(trackers.join('\n'));
    });

    addOnCloseCallback([=](){
         GlobalObjects::appSetting->setValue("Download/TrackerSubscriberAutoCheck", autoCheck->isChecked());
    });
    setSizeSettingKey("DialogSize/TrackerSubscribe", QSize(400*logicalDpiX()/96,300*logicalDpiY()/96));
    trackerSrcView->header()->resizeSection(0, width()/2);
}
