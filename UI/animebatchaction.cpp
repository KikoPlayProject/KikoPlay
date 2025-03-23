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
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"

AnimeBatchAction::AnimeBatchAction(AnimeModel *animeModel, QWidget *parent) :
    CFramelessDialog(tr("Batch Operation"), parent, false, true, false)
{
    QTreeView *animeView=new QTreeView(this);
    animeView->setRootIsDecorated(false);
    animeView->setAlternatingRowColors(true);
    animeView->setContextMenuPolicy(Qt::CustomContextMenu);
    animeView->setSortingEnabled(true);

    AnimeListModel *animeListModel = new AnimeListModel(animeModel, this);
    AnimeListProxyModel *proxyModel= new AnimeListProxyModel(this);
    proxyModel->setSourceModel(animeListModel);
    animeView->setModel(proxyModel);

    QPushButton *updateInfo = new KPushButton(tr("Update Info"),this);
    QPushButton *updateTags = new KPushButton(tr("Update Tag"),this);
    QPushButton *removeAnime = new KPushButton(tr("Remove"),this);
    QLineEdit *searchEdit = new ElaLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search"));
    searchEdit->setClearButtonEnabled(true);

    QGridLayout *animeBGLayout=new QGridLayout(this);
    animeBGLayout->addWidget(updateInfo,0,0);
    animeBGLayout->addWidget(updateTags,0,1);
    animeBGLayout->addWidget(removeAnime,0,2);
    animeBGLayout->addWidget(searchEdit,0,4);
    animeBGLayout->addWidget(animeView,1,0,1,5);
    animeBGLayout->setRowStretch(1,1);
    animeBGLayout->setColumnStretch(3,1);

    ElaMenu *actionMenu = new ElaMenu(animeView);
    QObject::connect(animeView, &QTreeView::customContextMenuRequested, this, [=](){
        actionMenu->exec(QCursor::pos());
    });
    QAction *actSelectAll = actionMenu->addAction(tr("Select All/Cancel"));
    QObject::connect(actSelectAll,&QAction::triggered, this, [=](){
       static bool checkAll = false;
       checkAll = !checkAll;
       animeListModel->setAllChecked(checkAll);
    });

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
