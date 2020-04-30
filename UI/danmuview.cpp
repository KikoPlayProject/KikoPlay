#include "danmuview.h"
#include <QTreeView>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include <QActionGroup>
#include <QToolButton>
#include <QMenu>
#include <QHeaderView>
#include <QWidgetAction>
#include "globalobjects.h"
#include "Play/Danmu/danmuviewmodel.h"
DanmuView::DanmuView(const QList<DanmuComment *> *danmuList, QWidget *parent, const QString &filterStr, DanmuFilterBox::FilterType type):CFramelessDialog (tr("View Danmu"),parent)
{
    initView(danmuList->count());
    DanmuViewModel<DanmuComment *> *model=new DanmuViewModel<DanmuComment *>(danmuList,this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setSourceModel(model);
    danmuView->setModel(proxyModel);
    QObject::connect(filterEdit,&DanmuFilterBox::filterChanged,[proxyModel,this](int type, const QString &keyword){
        proxyModel->setFilterKeyColumn(type);
        proxyModel->setFilterRegExp(keyword);
        tipLabel->setText(tr("Danmu Count: %1").arg(proxyModel->rowCount()));
    });
    if(!filterStr.isEmpty()) filterEdit->setFilter(type, filterStr);
}

DanmuView::DanmuView(const QList<QSharedPointer<DanmuComment> > *danmuList, QWidget *parent, const QString &filterStr, DanmuFilterBox::FilterType type):CFramelessDialog (tr("View Danmu"),parent)
{
    initView(danmuList->count());
    DanmuViewModel<QSharedPointer<DanmuComment> > *model=new DanmuViewModel<QSharedPointer<DanmuComment> >(danmuList,this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(4);
    proxyModel->setSourceModel(model);
    danmuView->setModel(proxyModel);
    QObject::connect(filterEdit,&DanmuFilterBox::filterChanged,[proxyModel,this](int type, const QString &keyword){
        proxyModel->setFilterKeyColumn(type);
        proxyModel->setFilterRegExp(keyword);
        tipLabel->setText(tr("Danmu Count: %1").arg(proxyModel->rowCount()));
    });
    if(!filterStr.isEmpty()) filterEdit->setFilter(type, filterStr);
}

void DanmuView::initView(int danmuCount)
{
    danmuView=new QTreeView(this);
    danmuView->setRootIsDecorated(false);
    danmuView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    danmuView->setAlternatingRowColors(true);
    danmuView->setSortingEnabled(true);
    danmuView->header()->setSortIndicator(0, Qt::SortOrder::AscendingOrder);

    tipLabel = new QLabel(tr("Danmu Count: %1").arg(danmuCount),this);
    filterEdit=new DanmuFilterBox(this);

    QGridLayout *viewGLayout=new QGridLayout(this);
    viewGLayout->addWidget(tipLabel,0,0);
    viewGLayout->addWidget(filterEdit,0,2);
    viewGLayout->addWidget(danmuView,1,0,1,3);
    viewGLayout->setRowStretch(1,1);
    viewGLayout->setColumnStretch(1,1);
    viewGLayout->setContentsMargins(0, 0, 0, 0);
    resize(GlobalObjects::appSetting->value("DialogSize/DanmuView",QSize(600*logicalDpiX()/96,400*logicalDpiY()/96)).toSize());
}

void DanmuView::onClose()
{
    GlobalObjects::appSetting->setValue("DialogSize/DanmuView",size());
    CFramelessDialog::onClose();
}

DanmuFilterBox::DanmuFilterBox(QWidget *parent): QLineEdit(parent)
  , filterTypeGroup(new QActionGroup(this))
{
    setClearButtonEnabled(true);

    QMenu *menu = new QMenu(this);

    filterTypeGroup->setExclusive(true);
    QAction *filterContent = menu->addAction(tr("Content"));
    filterContent->setData(QVariant(int(4)));
    filterContent->setCheckable(true);
    filterContent->setChecked(true);
    filterTypeGroup->addAction(filterContent);

    QAction *filterUser = menu->addAction(tr("User"));
    filterUser->setData(QVariant(int(2)));
    filterUser->setCheckable(true);
    filterTypeGroup->addAction(filterUser);

    QAction *filterType = menu->addAction(tr("Type"));
    filterType->setData(QVariant(int(1)));
    filterType->setCheckable(true);
    filterTypeGroup->addAction(filterType);

    QAction *filterTime = menu->addAction(tr("Time"));
    filterTime->setData(QVariant(int(0)));
    filterTime->setCheckable(true);
    filterTypeGroup->addAction(filterTime);

    connect(filterTypeGroup, &QActionGroup::triggered,[this](QAction *act){
        emit filterChanged(act->data().toInt(),this->text());
    });
    connect(this, &QLineEdit::textChanged, [this](const QString &text){
        emit filterChanged(filterTypeGroup->checkedAction()->data().toInt(),text);
    });

    QToolButton *optionsButton = new QToolButton;
    optionsButton->setCursor(Qt::ArrowCursor);

    optionsButton->setFocusPolicy(Qt::NoFocus);
    optionsButton->setObjectName(QStringLiteral("FilterOptionButton"));
    GlobalObjects::iconfont.setPointSize(14);
    optionsButton->setFont(GlobalObjects::iconfont);
    optionsButton->setText(QChar(0xe609));
    optionsButton->setMenu(menu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);

    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(optionsButton);
    addAction(optionsAction, QLineEdit::LeadingPosition);
}

void DanmuFilterBox::setFilter(FilterType type, const QString &filterStr)
{
    filterTypeGroup->actions()[type]->setChecked(true);
    setText(filterStr);
}
