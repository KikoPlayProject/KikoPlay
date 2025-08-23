#include "playlistpage.h"
#include "Common/notifier.h"
#include "MediaLibrary/animeprovider.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaToggleSwitch.h"
#include <QVBoxLayout>
#include "UI/inputdialog.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"
#include "Play/Playlist/playlist.h"
#include <QSettings>
#include <QLabel>
#include <QListWidget>
#ifdef KSERVICE
#include "Service/kservice.h"
#endif


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

#ifdef KSERVICE
    ElaToggleSwitch *kserviceMatchSwitch = new ElaToggleSwitch(this);
    kserviceMatchSwitch->setIsToggled(KService::instance()->enableKServiceMatch());
    epMatchArea->addItem(tr("Use KService Matching"), kserviceMatchSwitch);

    QObject::connect(kserviceMatchSwitch, &ElaToggleSwitch::toggled, this, [=](bool checked){
        KService::instance()->setEnableKServiceMatch(checked);
    });

    KPushButton *kserviceLibraryOrderBtn = new KPushButton(tr("Edit"), this);
    epMatchArea->addItem(tr("KService Library Download Priority"), kserviceLibraryOrderBtn);
    QObject::connect(kserviceLibraryOrderBtn, &QPushButton::clicked, this, [=](){
        KLibraryOrderDialog dialog(this);
        dialog.exec();
    });
#endif


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
                                tr("Set rules line by line, supporting regular expressions.\nKikoPlay will skip items matched by the rules during the match process."),
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

#ifdef KSERVICE
KLibraryOrderDialog::KLibraryOrderDialog(QWidget *parent) : CFramelessDialog(tr("Library Download Priority"), parent, true)
{
    QLabel *tipLabel = new QLabel(tr("When multiple data sources are obtained from KService, set the download priority:\n(Drag to change the order)"), this);
    sourecOrderView = new QListWidget(this);
    sourecOrderView->setObjectName(QStringLiteral("SourceOrderView"));
    QFont f = font();
    f.setPointSize(12);
    sourecOrderView->setFont(f);
    sourecOrderView->setDragEnabled(true);
    sourecOrderView->setAcceptDrops(true);
    sourecOrderView->setDragDropMode(QAbstractItemView::InternalMove);
    sourecOrderView->setDropIndicatorShown(true);
    sourecOrderView->setDefaultDropAction(Qt::MoveAction);
    sourecOrderView->setSelectionMode(QListWidget::SingleSelection);

    auto librarySrc = KService::instance()->getLibrarySource();
    for (auto &src : librarySrc)
    {
        QListWidgetItem *item = new QListWidgetItem(src.first, sourecOrderView, src.second.first);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(src.second.second ? Qt::Checked : Qt::Unchecked);
    }

    QGridLayout *kGLayout = new QGridLayout(this);
    kGLayout->addWidget(tipLabel, 0, 0);
    kGLayout->addWidget(sourecOrderView, 1, 0);
    kGLayout->setRowStretch(1, 1);

    setSizeSettingKey("DialogSize/KLibraryOrderDialog",QSize(100, 120));
}

void KLibraryOrderDialog::onAccept()
{
    QList<QPair<int, bool>> indexSelected;
    bool hasChecked = false;
    for (int i = 0; i < sourecOrderView->count(); ++i)
    {
        int src = sourecOrderView->item(i)->type();
        bool checked = sourecOrderView->item(i)->checkState() == Qt::Checked;
        indexSelected.append({src, checked});
        if (checked) hasChecked = true;
    }
    if (!hasChecked)
    {
        showMessage(tr("At least one source must be selected"), NM_ERROR | NM_HIDE);
        return;
    }
    KService::instance()->setLibrarySourceIndex(indexSelected);
    CFramelessDialog::onAccept();
}
#endif
