#include "animebatchaction.h"
#include <QTreeView>
#include <QPushButton>
#include <QAction>
#include <QLineEdit>
#include <QGridLayout>
#include <QHeaderView>
#include <QSettings>
#include <QMessageBox>
#include "MediaLibrary/animelistmodel.h"
#include "globalobjects.h"
#include "Common/notifier.h"

AnimeBatchAction::AnimeBatchAction(AnimeModel *animeModel, QWidget *parent) :
    CFramelessDialog(tr("Batch Operation"), parent, false, true, false)
{
    QTreeView *animeView=new QTreeView(this);
    animeView->setRootIsDecorated(false);
    animeView->setAlternatingRowColors(true);
    animeView->setContextMenuPolicy(Qt::ActionsContextMenu);
    animeView->setSortingEnabled(true);

    AnimeListModel *animeListModel = new AnimeListModel(animeModel, this);
    AnimeListProxyModel *proxyModel= new AnimeListProxyModel(this);
    proxyModel->setSourceModel(animeListModel);
    animeView->setModel(proxyModel);

    QPushButton *updateInfo = new QPushButton(tr("Update Info"),this);
    QPushButton *updateTags = new QPushButton(tr("Update Tag"),this);
    QPushButton *removeAnime = new QPushButton(tr("Remove"),this);
    QLineEdit *searchEdit = new QLineEdit(this);
    searchEdit->setClearButtonEnabled(true);

    QGridLayout *animeBGLayout=new QGridLayout(this);
    animeBGLayout->addWidget(updateInfo,0,0);
    animeBGLayout->addWidget(updateTags,0,1);
    animeBGLayout->addWidget(removeAnime,0,2);
    animeBGLayout->addWidget(searchEdit,0,4);
    animeBGLayout->addWidget(animeView,1,0,1,5);
    animeBGLayout->setRowStretch(1,1);
    animeBGLayout->setColumnStretch(3,1);
    animeBGLayout->setContentsMargins(0, 0, 0, 0);

    QAction *actSelectAll=new QAction(tr("Select All/Cancel"),this);
    QObject::connect(actSelectAll,&QAction::triggered, this, [=](){
       static bool checkAll = false;
       checkAll = !checkAll;
       animeListModel->setAllChecked(checkAll);
    });
    animeView->addAction(actSelectAll);

    QObject::connect(animeListModel, &AnimeListModel::showMessage, this, &AnimeBatchAction::showMessage);

    QObject::connect(updateInfo, &QPushButton::clicked, this, [=](){
        showBusyState(true);
        animeListModel->updateCheckedInfo();
        showBusyState(false);
    });

    QObject::connect(updateTags, &QPushButton::clicked, this, [=](){
        showBusyState(true);
        animeListModel->updateCheckedTag();
        showBusyState(false);
    });

    QObject::connect(removeAnime, &QPushButton::clicked, this, [=](){
        int c = animeListModel->checkedCount();
        if(c==0) return;
        QMessageBox::StandardButton btn = QMessageBox::information(this,tr("Remove"),tr("Are you sure you want to remove these %1 anime(s)?").arg(c),
                                 QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel);
        if(btn==QMessageBox::Cancel)return;
        showBusyState(true);
        animeListModel->removeChecked();
        showBusyState(false);
    });

    QObject::connect(searchEdit, &QLineEdit::textChanged, this, [=](const QString &text){
        proxyModel->setFilterKeyColumn(static_cast<int>(AnimeListModel::Columns::TITLE));
        proxyModel->setFilterRegExp(text);
    });

    QVariant headerState(GlobalObjects::appSetting->value("HeaderViewState/AnimeBatchView"));
    if(!headerState.isNull())
        animeView->header()->restoreState(headerState.toByteArray());

    addOnCloseCallback([animeView](){
        GlobalObjects::appSetting->setValue("HeaderViewState/AnimeBatchView", animeView->header()->saveState());
    });
    setSizeSettingKey("DialogSize/AnimeBatchAction",QSize(600*logicalDpiX()/96,400*logicalDpiY()/96));

}
