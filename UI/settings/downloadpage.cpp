#include "downloadpage.h"
#include <QPlainTextEdit>
#include <QTextEdit>
#include <QLabel>
#include <QSettings>
#include <QGridLayout>
#include <QSyntaxHighlighter>
#include <climits>
#include "UI/ela/ElaSpinBox.h"
#include "UI/ela/ElaToggleSwitch.h"
#include "UI/dialogs/trackersubscribedialog.h"
#include "UI/widgets/kplaintextedit.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"


#define SETTING_KEY_DOWNLOAD_MAX_SPEED "Download/MaxDownloadLimit"
#define SETTING_KEY_UPLOAD_MAX_SPEED "Download/MaxUploadLimit"
#define SETTING_KEY_SEED_TIME "Download/SeedTime"
#define SETTING_KEY_MAX_CONCURRENT "Download/ConcurrentDownloads"
#define SETTING_KEY_AUTO_ADD_TO_PLAYLIST "Download/AutoAddToList"
#define SETTING_KEY_SKIP_MANGET_FILE_SELECT "Download/SkipMagnetFileSelect"
#define SETTING_KEY_KILL_EXIST_ARIA2 "Download/KillExistAria2"
#define SETTING_KEY_TRACKERS "Download/Trackers"
#define SETTING_KEY_ARIA2_ARGS "Download/Aria2Args"

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
            //int index= commentRe.indexIn(text);
            auto match = commentRe.match(text);
            if (match.hasMatch())
            // if(index!=-1)
            {
                setFormat(match.capturedStart(0), match.capturedLength(0), commentFormat);
                return;
            }
            //index = optionRe.indexIn(text);
            match = optionRe.match(text);
            //if (index>-1)
            if (match.hasMatch())
            {
                QStringList opList = match.capturedTexts();
                int opNamePos = match.capturedStart(1), opContentPos = match.capturedStart(2);
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
        QRegularExpression optionRe,commentRe;
    };
}


DownloadPage::DownloadPage(QWidget *parent) : SettingPage(parent)
{
    SettingItemArea *limitArea = new SettingItemArea(tr("Limit"), this);

    ElaSpinBox *maxDownSpeedSpin = new ElaSpinBox(this);
    maxDownSpeedSpin->setRange(0, INT_MAX);
    maxDownSpeedSpin->setValue(GlobalObjects::appSetting->value(SETTING_KEY_DOWNLOAD_MAX_SPEED, 0).toInt());
    limitArea->addItem(tr("Max Download Speed(KB)"), maxDownSpeedSpin);

    ElaSpinBox *maxUpSpeedSpin = new ElaSpinBox(this);
    maxUpSpeedSpin->setRange(0, INT_MAX);
    maxUpSpeedSpin->setValue(GlobalObjects::appSetting->value(SETTING_KEY_UPLOAD_MAX_SPEED, 0).toInt());
    limitArea->addItem(tr("Max Upload Speed(KB)"), maxUpSpeedSpin);

    ElaSpinBox *seedTimeSpin = new ElaSpinBox(this);
    seedTimeSpin->setRange(0, INT_MAX);
    seedTimeSpin->setValue(GlobalObjects::appSetting->value(SETTING_KEY_SEED_TIME, 5).toInt());
    limitArea->addItem(tr("Seed Time(min)"), seedTimeSpin);

    ElaSpinBox *maxConcurrentSpin = new ElaSpinBox(this);
    maxConcurrentSpin->setMinimum(0);
    maxConcurrentSpin->setValue(GlobalObjects::appSetting->value(SETTING_KEY_MAX_CONCURRENT, 5).toInt());
    limitArea->addItem(tr("Max Concurrent Downloads"), maxConcurrentSpin);


    SettingItemArea *behaviorArea = new SettingItemArea(tr("Behavior"), this);

    ElaToggleSwitch *autoAddtoPlaylistSwitch = new ElaToggleSwitch(this);
    autoAddtoPlaylistSwitch->setIsToggled(GlobalObjects::appSetting->value(SETTING_KEY_AUTO_ADD_TO_PLAYLIST, true).toBool());
    behaviorArea->addItem(tr("Auto-Add to Playlist on Download Completion"), autoAddtoPlaylistSwitch);

    ElaToggleSwitch *skipMagnetFileSelectSwitch = new ElaToggleSwitch(this);
    skipMagnetFileSelectSwitch->setIsToggled(GlobalObjects::appSetting->value(SETTING_KEY_SKIP_MANGET_FILE_SELECT, false).toBool());
    behaviorArea->addItem(tr("Skip Magnet File Selection"), skipMagnetFileSelectSwitch);


    SettingItemArea *aria2Area = new SettingItemArea(tr("Aria2"), this);

    ElaToggleSwitch *killExistAria2Switch = new ElaToggleSwitch(this);
    killExistAria2Switch->setIsToggled(GlobalObjects::appSetting->value(SETTING_KEY_KILL_EXIST_ARIA2, true).toBool());
    aria2Area->addItem(tr("Kill existing aria2 processes at startup"), killExistAria2Switch);

    KPushButton *trackerBtn = new KPushButton(tr("Edit"), this);
    aria2Area->addItem(tr("Extra BT Trackers"), trackerBtn);

    KPushButton *trackerSubscribeBtn = new KPushButton(tr("Edit"), this);
    aria2Area->addItem(tr("Tracker Subscribe"), trackerSubscribeBtn);

    KPushButton *aria2ArgBtn = new KPushButton(tr("Edit"), this);
    aria2Area->addItem(tr("Aria2 Startup Args"), aria2ArgBtn);

    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    itemVLayout->addWidget(limitArea);
    itemVLayout->addWidget(behaviorArea);
    itemVLayout->addWidget(aria2Area);
    itemVLayout->addStretch(1);


    QObject::connect(maxDownSpeedSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val){
        changedValues["downSpeed"] = val;
        GlobalObjects::appSetting->setValue(SETTING_KEY_DOWNLOAD_MAX_SPEED, val);
    });

    QObject::connect(maxUpSpeedSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val){
        changedValues["upSpeed"] = val;
        GlobalObjects::appSetting->setValue(SETTING_KEY_UPLOAD_MAX_SPEED, val);
    });

    QObject::connect(seedTimeSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val){
        changedValues["seedTime"] = val;
        GlobalObjects::appSetting->setValue(SETTING_KEY_SEED_TIME, val);
    });

    QObject::connect(maxConcurrentSpin, QOverload<int>::of(&QSpinBox::valueChanged), this, [=](int val){
        changedValues["concurrent"] = val;
        GlobalObjects::appSetting->setValue(SETTING_KEY_MAX_CONCURRENT, val);
    });

    QObject::connect(autoAddtoPlaylistSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::appSetting->setValue(SETTING_KEY_AUTO_ADD_TO_PLAYLIST, checked);
    });

    QObject::connect(skipMagnetFileSelectSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::appSetting->setValue(SETTING_KEY_SKIP_MANGET_FILE_SELECT, checked);
    });

    QObject::connect(killExistAria2Switch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::appSetting->setValue(SETTING_KEY_KILL_EXIST_ARIA2, checked);
    });

    QObject::connect(trackerBtn, &KPushButton::clicked, this, [=](){
        TrackerEditDialog dialog(this);
        if (QDialog::Accepted == dialog.exec())
        {
            if (dialog.manualTrackerListChanged)
            {
                changedValues["btTracker"] = GlobalObjects::appSetting->value(SETTING_KEY_TRACKERS, QStringList()).toStringList();
            }
        }
    });

    QObject::connect(trackerSubscribeBtn, &KPushButton::clicked, this, [=](){
        TrackerSubscribeDialog dialog(this);
        dialog.exec();
    });

    QObject::connect(aria2ArgBtn, &KPushButton::clicked, this, [=](){
        Aria2OptionEditDialog dialog(this);
        dialog.exec();
    });
}


TrackerEditDialog::TrackerEditDialog(QWidget *parent) : CFramelessDialog(tr("Extra Tracker"), parent, true)
{
    btTrackers = new KPlainTextEdit(this);
    btTrackers->appendPlainText(GlobalObjects::appSetting->value(SETTING_KEY_TRACKERS, QStringList()).toStringList().join('\n'));
    QObject::connect(btTrackers, &QPlainTextEdit::textChanged, this, [this](){
        manualTrackerListChanged = true;
    });

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(btTrackers);
    setSizeSettingKey("DialogSize/TrackerEditDialog", QSize(400, 300));
}

void TrackerEditDialog::onAccept()
{
    if (btTrackers && manualTrackerListChanged)
    {
        const QStringList trackers = btTrackers->toPlainText().split('\n', Qt::SkipEmptyParts);
        GlobalObjects::appSetting->setValue(SETTING_KEY_TRACKERS, trackers);
    }
    CFramelessDialog::onAccept();
}

Aria2OptionEditDialog::Aria2OptionEditDialog(QWidget *parent) : CFramelessDialog(tr("Aria2 Startup Args"), parent, true)
{
    argEdit = new QTextEdit(this);
    argEdit->setFont(QFont("Consolas", 12));
    new OptionHighLighter(argEdit->document());
    argEdit->setPlainText(GlobalObjects::appSetting->value(SETTING_KEY_ARIA2_ARGS, "").toString());
    QObject::connect(argEdit, &QTextEdit::textChanged, this, [this](){
        argChanged = true;
    });

    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->addWidget(argEdit);
    setSizeSettingKey("DialogSize/Aria2OptionEditDialog", QSize(400, 300));
}

void Aria2OptionEditDialog::onAccept()
{
    if (argEdit && argChanged)
    {
        GlobalObjects::appSetting->setValue(SETTING_KEY_ARIA2_ARGS, argEdit->toPlainText());
    }
}
