#include "playerpage.h"
#include <QLabel>
#include <QLabel>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QSettings>
#include <QTabWidget>
#include <QPushButton>
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaToggleSwitch.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#include "UI/dialogs/mpvconfediror.h"


PlayerPage::PlayerPage(QWidget *parent) : SettingPage(parent)
{
    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    itemVLayout->addWidget(initBehaviorArea());
    itemVLayout->addWidget(initMpvArea());
    itemVLayout->addStretch(1);
}

SettingItemArea *PlayerPage::initBehaviorArea()
{
    SettingItemArea *behaviorArea = new SettingItemArea(tr("Behavior"), this);

    ElaComboBox *clickBehaviorCombo = new ElaComboBox(this);
    clickBehaviorCombo->addItems({tr("Play/Pause"), tr("Show/Hide PlayControl")});
    clickBehaviorCombo->setCurrentIndex(GlobalObjects::mpvplayer->getClickBehavior());
    behaviorArea->addItem(tr("Click Behavior"), clickBehaviorCombo);

    ElaComboBox *dbClickBehaviorCombo = new ElaComboBox(this);
    dbClickBehaviorCombo->addItems({tr("FullScreen"), tr("Play/Pause")});
    dbClickBehaviorCombo->setCurrentIndex(GlobalObjects::mpvplayer->getDbClickBehavior());
    behaviorArea->addItem(tr("Double Click Behavior"), dbClickBehaviorCombo);

    ElaToggleSwitch *showPreviewSwitch = new ElaToggleSwitch(this);
    showPreviewSwitch->setIsToggled(GlobalObjects::mpvplayer->getShowPreview());
    behaviorArea->addItem(tr("Show Preview Over ProgressBar(Restart required)"), showPreviewSwitch);

    ElaToggleSwitch *showRecentlySwitch = new ElaToggleSwitch(this);
    showRecentlySwitch->setIsToggled(GlobalObjects::mpvplayer->getShowRecent());
    behaviorArea->addItem(tr("Show Recently Played Files"), showRecentlySwitch);

    QObject::connect(clickBehaviorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [](int index){
        GlobalObjects::mpvplayer->setClickBehavior(index);
    });

    QObject::connect(dbClickBehaviorCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [](int index){
        GlobalObjects::mpvplayer->setDbClickBehavior(index);
    });

    QObject::connect(showPreviewSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::mpvplayer->setShowPreview(checked);
    });

    QObject::connect(showRecentlySwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::mpvplayer->setShowRecent(checked);
    });

    return behaviorArea;
}

SettingItemArea *PlayerPage::initMpvArea()
{
    SettingItemArea *mpvArea = new SettingItemArea(tr("MPV"), this);

    KPushButton *confEditBtn = new KPushButton(tr("Edit"), this);
    mpvArea->addItem(tr("Player Configuration"), confEditBtn);


    QObject::connect(confEditBtn, &KPushButton::clicked, this, [=](){
        MPVConfEdiror editor(this);
        editor.exec();
    });

    return mpvArea;
}

