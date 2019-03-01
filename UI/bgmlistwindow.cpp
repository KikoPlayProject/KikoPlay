#include "bgmlistwindow.h"
#include "Download/BgmList/bgmlist.h"
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QButtonGroup>
#include <QAction>
#include <QTreeView>
#include <QHeaderView>
#include "globalobjects.h"
#include "MediaLibrary/animelibrary.h"
BgmListWindow::BgmListWindow(QWidget *parent) : QWidget(parent)
{
    bgmList=new BgmList(this);
    BgmListFilterProxyModel *bgmListProxyModel=new BgmListFilterProxyModel(this);
    bgmListProxyModel->setSourceModel(bgmList);
    QHBoxLayout *btnHLayout=new QHBoxLayout();
    QButtonGroup *filterButtonGroup=new QButtonGroup(this);
    QString btnTitles[]={tr("Sun"),tr("Mon"),tr("Tue"),tr("Wed"),tr("Thu"),tr("Fri"),tr("Sat"),tr("All")};
    int curWeekDay=QDate::currentDate().dayOfWeek()%7;
    for(int i=0;i<8;++i)
    {
        QPushButton *filterBtn=new QPushButton(btnTitles[i]+(i==curWeekDay?tr("(Today)"):""),this);
        filterBtn->setCheckable(true);
        filterBtn->setObjectName(QStringLiteral("BgmFilterBtn"));
        btnHLayout->addWidget(filterBtn);
        filterButtonGroup->addButton(filterBtn,i);
    }
    QPushButton *focusBtn=new QPushButton(tr("Focus"),this);
    focusBtn->setCheckable(true);
    focusBtn->setObjectName(QStringLiteral("BgmFilterBtn"));
    btnHLayout->addWidget(focusBtn);
    QObject::connect(focusBtn,&QPushButton::clicked,this,[bgmListProxyModel](bool checked){
       bgmListProxyModel->setFocusFilter(checked);
    });
    QObject::connect(filterButtonGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[bgmListProxyModel,focusBtn](int id, bool checked){
        if(checked) bgmListProxyModel->setWeekFilter(id);
    });
    filterButtonGroup->button(curWeekDay)->setChecked(true);
    btnHLayout->addStretch(1);
    QLabel *infoLabel=new QLabel(this);
    btnHLayout->addWidget(infoLabel);
    QPushButton *refreshBtn=new QPushButton(tr("Refresh"),this);
    btnHLayout->addWidget(refreshBtn);

    QLabel *bgmLabel=new QLabel("<a href = \"https://bgmlist.com/\">BgmList</a>", this);
    bgmLabel->setOpenExternalLinks(true);
    btnHLayout->addWidget(bgmLabel);
    QObject::connect(refreshBtn,&QPushButton::clicked,this,[this,bgmListProxyModel](){
       bgmList->refreshData(true);
       bgmListProxyModel->invalidate();
    });
    QObject::connect(bgmList,&BgmList::bgmStatusUpdated,this,[infoLabel,refreshBtn](int type,const QString &msg){
        infoLabel->setText(msg);
        refreshBtn->setVisible(type==3);
    });

    bgmListView=new QTreeView(this);
    bgmListView->setModel(bgmListProxyModel);
    bgmListView->setObjectName(QStringLiteral("BgmListView"));
    bgmListView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    bgmListView->header()->setObjectName(QStringLiteral("BgmListHeader"));
    bgmListView->header()->setDefaultAlignment(Qt::AlignCenter);
    bgmListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bgmListView->setFont(QFont("Microsoft YaHei UI",12));
    bgmListView->setIndentation(0);
    bgmListView->setContextMenuPolicy(Qt::ActionsContextMenu);

    QAction *addToLibrary=new QAction(tr("Add To Library"), this);
    QObject::connect(addToLibrary,&QAction::triggered,this,[this,bgmListProxyModel](){
        QItemSelection selection = bgmListProxyModel->mapSelectionToSource(bgmListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        const BgmItem &item=bgmList->bgmList().at(selection.indexes().last().row());
        GlobalObjects::library->addToLibrary(item.title,item.bgmId);
    });
    bgmListView->addAction(addToLibrary);

    QGridLayout *bgmWindowGLayout=new QGridLayout(this);
    bgmWindowGLayout->addLayout(btnHLayout,0,0);
    bgmWindowGLayout->addWidget(bgmListView,1,0);
}

void BgmListWindow::showEvent(QShowEvent *)
{
    QTimer::singleShot(0,[this](){
        bgmList->init();
    });
}

void BgmListWindow::resizeEvent(QResizeEvent *)
{
    int oneWidth = bgmListView->width() / 10;
    bgmListView->header()->resizeSection(0,4*oneWidth);
    bgmListView->header()->resizeSection(1,2*oneWidth);
    bgmListView->header()->resizeSection(2,3*oneWidth);
    bgmListView->header()->resizeSection(3,1*oneWidth);
}



