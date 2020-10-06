#include "bgmlistwindow.h"
#include "Download/BgmList/bgmlist.h"
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QButtonGroup>
#include <QAction>
#include <QTreeView>
#include <QHeaderView>
#include <QMenu>
#include <QDesktopServices>
#include <QComboBox>
#include <QApplication>
#include <QStyledItemDelegate>
#include "globalobjects.h"
#include "MediaLibrary/animelibrary.h"
namespace
{
    class TextColorDelegate: public QStyledItemDelegate
    {
    public:
        explicit TextColorDelegate(QObject* parent = nullptr) : QStyledItemDelegate(parent)
        { }

        void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
        {
            QStyleOptionViewItem ViewOption(option);
            QColor itemForegroundColor = index.data(Qt::ForegroundRole).value<QColor>();
            if (itemForegroundColor.isValid())
            {
                if (itemForegroundColor != option.palette.color(QPalette::WindowText))
                    ViewOption.palette.setColor(QPalette::HighlightedText, itemForegroundColor);

            }
            QStyledItemDelegate::paint(painter, ViewOption, index);
        }
    };
}
BgmListWindow::BgmListWindow(QWidget *parent) : QWidget(parent)
{
    bgmList=new BgmList(this);
    bgmList->setObjectName(QStringLiteral("bgmListModel"));
    BgmListFilterProxyModel *bgmListProxyModel=new BgmListFilterProxyModel(this);
    bgmListProxyModel->setSourceModel(bgmList);
    QHBoxLayout *btnHLayout=new QHBoxLayout();
    QButtonGroup *filterButtonGroup=new QButtonGroup(this);
    weekDay=QDate::currentDate().dayOfWeek()%7;
    for(int i=0;i<8;++i)
    {
        QPushButton *filterBtn=new QPushButton(btnTitles[i]+(i==weekDay?tr("(Today)"):""),this);
        filterBtn->setCheckable(true);
        filterBtn->setObjectName(QStringLiteral("BgmFilterBtn"));
        btnHLayout->addWidget(filterBtn);
        filterButtonGroup->addButton(filterBtn,i);
        weekDayBtnList.append(filterBtn);
    }
    QPushButton *focusBtn=new QPushButton(tr("Focus"),this);
    focusBtn->setCheckable(true);
    focusBtn->setObjectName(QStringLiteral("BgmFilterBtn"));
    btnHLayout->addWidget(focusBtn);
    QObject::connect(focusBtn,&QPushButton::clicked,this,[bgmListProxyModel](bool checked){
       bgmListProxyModel->setFocusFilter(checked);
    });
    QPushButton *newBtn=new QPushButton(tr("New"),this);
    newBtn->setCheckable(true);
    newBtn->setObjectName(QStringLiteral("BgmFilterBtn"));
    btnHLayout->addWidget(newBtn);
    QObject::connect(newBtn,&QPushButton::clicked,this,[bgmListProxyModel](bool checked){
       bgmListProxyModel->setNewBgmFilter(checked);
    });

    QObject::connect(filterButtonGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[bgmListProxyModel,focusBtn](int id, bool checked){
        if(checked) bgmListProxyModel->setWeekFilter(id);
    });
    filterButtonGroup->button(weekDay)->setChecked(true);
    btnHLayout->addStretch(1);

    QComboBox *seasonIdCombo=new QComboBox(this);
    seasonIdCombo->setProperty("cScrollStyle", true);
    seasonIdCombo->view()->setMinimumWidth(seasonIdCombo->view()->fontMetrics().width("0000-00") +
                                           QApplication::style()->pixelMetric(QStyle::PixelMetric::PM_ScrollBarExtent) +
                                           seasonIdCombo->view()->autoScrollMargin());
    seasonIdCombo->addItems(bgmList->seasonList());
    QObject::connect(seasonIdCombo, (void (QComboBox::*)(const QString &))&QComboBox::currentIndexChanged,this,[this,seasonIdCombo](const QString &id){
        seasonIdCombo->setEnabled(false);
        bgmListView->setEnabled(false);
        bgmList->setSeason(id);
        seasonIdCombo->setEnabled(true);
        bgmListView->setEnabled(true);
    });
    QObject::connect(bgmList, &BgmList::seasonsUpdated, this, [this,seasonIdCombo](){
       seasonIdCombo->clear();
       seasonIdCombo->addItems(bgmList->seasonList());
       seasonIdCombo->setCurrentIndex(seasonIdCombo->count()-1);
    });
    btnHLayout->addWidget(seasonIdCombo);

    QLabel *infoLabel=new QLabel(this);
    infoLabel->setObjectName(QStringLiteral("BgmInfoLabel"));
    btnHLayout->addWidget(infoLabel);
    QPushButton *refreshBtn=new QPushButton(tr("Refresh"),this);
    btnHLayout->addWidget(refreshBtn);

    QObject::connect(refreshBtn,&QPushButton::clicked,this,[this,bgmListProxyModel,seasonIdCombo](){
        seasonIdCombo->setEnabled(false);
        bgmList->refresh();
        bgmListProxyModel->invalidate();
        seasonIdCombo->setEnabled(true);
    });
    QObject::connect(bgmList,&BgmList::bgmStatusUpdated,this,[infoLabel,refreshBtn](int type,const QString &msg){
        infoLabel->setText(msg);
        refreshBtn->setVisible(type==3);
    });

    bgmListView=new BgmTreeView(this);
    bgmListView->setModel(bgmListProxyModel);
    bgmListView->setItemDelegate(new TextColorDelegate(this));
    bgmListView->setObjectName(QStringLiteral("BgmListView"));
    bgmListView->setSelectionMode(QAbstractItemView::SelectionMode::SingleSelection);
    bgmListView->header()->setObjectName(QStringLiteral("BgmListHeader"));
    bgmListView->header()->setDefaultAlignment(Qt::AlignCenter);
    bgmListView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    bgmListView->setFont(QFont(GlobalObjects::normalFont,12));
    bgmListView->setIndentation(0);
    bgmListView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(bgmListView, &QTreeView::doubleClicked,[this,bgmListProxyModel](const QModelIndex &index){
        const BgmItem &item=bgmList->bgmList().at(bgmListProxyModel->mapToSource(index).row());
        emit searchBgm(item.title);
    });
    QObject::connect(bgmListView, &BgmTreeView::normColorChanged, bgmList, &BgmList::setNormColor);
    QObject::connect(bgmListView, &BgmTreeView::hoverColorChanged, bgmList, &BgmList::setHoverColor);

    QAction *addToLibrary=new QAction(tr("Add To Library"), this);
    QObject::connect(addToLibrary,&QAction::triggered,this,[this,bgmListProxyModel](){
        QItemSelection selection = bgmListProxyModel->mapSelectionToSource(bgmListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        const BgmItem &item=bgmList->bgmList().at(selection.indexes().last().row());
        GlobalObjects::library->addToLibrary(item.title,item.bgmId);
    });
    QAction *onBangumi=new QAction(tr("Bangumi Info"), this);
    QObject::connect(onBangumi,&QAction::triggered,this,[this,bgmListProxyModel](){
        QItemSelection selection = bgmListProxyModel->mapSelectionToSource(bgmListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        const BgmItem &item=bgmList->bgmList().at(selection.indexes().last().row());
        QDesktopServices::openUrl(QUrl(QString("http://bgm.tv/subject/%1").arg(item.bgmId)));
    });
    QMenu *bgmContextMenu=new QMenu(this);
    QObject::connect(bgmListView,&QTreeView::customContextMenuRequested,[this,bgmContextMenu,onBangumi,addToLibrary,bgmListProxyModel](){
        QItemSelection selection = bgmListProxyModel->mapSelectionToSource(bgmListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        const BgmItem &item=bgmList->bgmList().at(selection.indexes().last().row());
        bgmContextMenu->clear();
        bgmContextMenu->addAction(addToLibrary);
        bgmContextMenu->addAction(onBangumi);
        QStringList sites(item.sitesName.split('|',QString::SkipEmptyParts));
        if(sites.count()>0)
        {
            bgmContextMenu->addSeparator();
            for(int i=0;i<sites.count();++i)
            {
                QAction *siteAction=new QAction(sites.at(i),bgmContextMenu);
                QString url(item.onAirURL.at(i));
                QObject::connect(siteAction,&QAction::triggered,siteAction,[url](){
                   QDesktopServices::openUrl(QUrl(url));
                });
                bgmContextMenu->addAction(siteAction);
            }
        }
        bgmContextMenu->exec(QCursor::pos());
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
        int cWeekDay=QDate::currentDate().dayOfWeek()%7;
        if(weekDay!=cWeekDay)
        {
            weekDayBtnList[weekDay]->setText(btnTitles[weekDay]);
            weekDay=cWeekDay;
            weekDayBtnList[weekDay]->setText(btnTitles[weekDay]+tr("(Today)"));
        }
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



