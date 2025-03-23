#include "playlistpage.h"
#include "MediaLibrary/animeprovider.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaToggleSwitch.h"
#include <QVBoxLayout>
#include "UI/inputdialog.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "Play/Playlist/playlist.h"
#include <QSettings>


PlaylistPage::PlaylistPage(QWidget *parent)
{
    QVBoxLayout *itemVLayout = new QVBoxLayout(this);
    itemVLayout->setSpacing(8);
    itemVLayout->addWidget(initEpMatchArea());
    itemVLayout->addWidget(initOtherArea());
    itemVLayout->addStretch(1);
}

SettingItemArea *PlaylistPage::initEpMatchArea()
{
    SettingItemArea *epMatchArea = new SettingItemArea(tr("Episode Matching"), this);

    ElaToggleSwitch *autoMatchSwitch = new ElaToggleSwitch(this);
    autoMatchSwitch->setIsToggled(GlobalObjects::playlist->isAutoMatch());
    epMatchArea->addItem(tr("Auto Epsidoe Matching"), autoMatchSwitch);

    ElaComboBox *defaultMatchCombo = new ElaComboBox(this);
    auto matchProviders = GlobalObjects::animeProvider->getMatchProviders();
    QString defaultSctiptId = GlobalObjects::animeProvider->defaultMatchScript();
    int defalutIndex = -1, curIndex = 0;
    for (const auto &p : matchProviders)
    {
        defaultMatchCombo->addItem(p.first, p.second);
        if (p.second == defaultSctiptId)
        {
            defalutIndex = curIndex;
        }
        ++curIndex;
    }
    defaultMatchCombo->setCurrentIndex(defalutIndex);
    epMatchArea->addItem(tr("Default Match Script"), defaultMatchCombo);

    KPushButton *matchFilterBtn = new KPushButton(tr("Edit"), this);
    epMatchArea->addItem(tr("Match Filter Setting"), matchFilterBtn);


    QObject::connect(autoMatchSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::playlist->setAutoMatch(checked);
    });

    QObject::connect(defaultMatchCombo, (void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, this, [=](int index){
        const QString scriptId = defaultMatchCombo->currentData().toString();
        if (!scriptId.isEmpty())
        {
            GlobalObjects::animeProvider->setDefaultMatchScript(scriptId);
        }
    });

    QObject::connect(matchFilterBtn, &QPushButton::clicked, this, [=](){
        InputDialog inputDialog(tr("Macth Filter"),
                                tr("Set rules line by line, supporting wildcards.\nKikoPlay will skip items matched by the rules during the match process."),
                                GlobalObjects::playlist->matchFilters(),
                                true,this, "DialogSize/MatchFilter");
        if (QDialog::Accepted == inputDialog.exec())
        {
            GlobalObjects::playlist->setMatchFilters(inputDialog.text.trimmed());
        }
    });

    return epMatchArea;
}

SettingItemArea *PlaylistPage::initOtherArea()
{
    SettingItemArea *otherArea = new SettingItemArea(tr("Other"), this);


    ElaToggleSwitch *addExternalSwitch = new ElaToggleSwitch(this);
    addExternalSwitch->setIsToggled(GlobalObjects::playlist->isAddExternal());
    otherArea->addItem(tr("Auto-add when opening external files"), addExternalSwitch);

    QObject::connect(addExternalSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        GlobalObjects::playlist->setAddExternal(checked);
    });


    return otherArea;
}
