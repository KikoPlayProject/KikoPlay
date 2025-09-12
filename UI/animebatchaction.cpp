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
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"

AnimeBatchAction::AnimeBatchAction(AnimeModel *animeModel, QWidget *parent) :
    CFramelessDialog(tr("Batch Operation"), parent, false, true, false)
{
    QTreeView *animeView=new QTreeView(this);
    animeView->setRootIsDecorated(false);
    animeView->setAlternatingRowColors(true);
    animeView->setContextMenuPolicy(Qt::CustomContextMenu);
    animeView->setItemDelegate(new KTreeviewItemDelegate(animeView));
    animeView->setSortingEnabled(true);
    animeView->setFont(QFont(GlobalObjects::normalFont, 11));

    AnimeListModel *animeListModel = new AnimeListModel(animeModel, this);
    AnimeListProxyModel *proxyModel= new AnimeListProxyModel(this);
    proxyModel->setSourceModel(animeListModel);
    animeView->setModel(proxyModel);

    QPushButton *selectAll = new KPushButton(tr("Select All/Cancel"),this);
    QPushButton *updateInfo = new KPushButton(tr("Update Info"),this);
    QPushButton *updateTags = new KPushButton(tr("Update Tag"),this);
    QPushButton *removeAnime = new KPushButton(tr("Remove"),this);
    QLineEdit *searchEdit = new ElaLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search"));
    searchEdit->setClearButtonEnabled(true);

    QGridLayout *animeBGLayout=new QGridLayout(this);
    animeBGLayout->addWidget(selectAll,0,0);
    animeBGLayout->addWidget(updateInfo,0,1);
    animeBGLayout->addWidget(updateTags,0,2);
    animeBGLayout->addWidget(removeAnime,0,3);
    animeBGLayout->addWidget(searchEdit,0,5);
    animeBGLayout->addWidget(animeView,1,0,1,6);
    animeBGLayout->setRowStretch(1,1);
    animeBGLayout->setColumnStretch(4,1);

    QObject::connect(animeListModel, &AnimeListModel::showMessage, this, &AnimeBatchAction::showMessage);

    QObject::connect(selectAll, &QPushButton::clicked, this, [=](){
        static bool checkAll = false;
        checkAll = !checkAll;
        for (int row = 0; row < proxyModel->rowCount(); ++row)
        {
            QModelIndex proxyIndex = proxyModel->index(row, 0);
            proxyModel->setData(proxyIndex, checkAll? Qt::CheckState::Checked : Qt::CheckState::Unchecked, Qt::CheckStateRole);
        }
    });

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
        proxyModel->setFilterRegularExpression(text);
    });

    QVariant headerState(GlobalObjects::appSetting->value("HeaderViewState/AnimeBatchView"));
    if(!headerState.isNull())
        animeView->header()->restoreState(headerState.toByteArray());

    addOnCloseCallback([animeView](){
        GlobalObjects::appSetting->setValue("HeaderViewState/AnimeBatchView", animeView->header()->saveState());
    });
    setSizeSettingKey("DialogSize/AnimeBatchAction",QSize(600*logicalDpiX()/96,400*logicalDpiY()/96));

}
