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
#include <QStyledItemDelegate>
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "globalobjects.h"
#include "Play/Danmu/danmuviewmodel.h"
#include "qapplication.h"


DanmuView::DanmuView(const QVector<DanmuComment *> *danmuList, QWidget *parent, int sourceId):CFramelessDialog (tr("View Danmu"),parent)
{
    initView();
    DanmuViewModel<DanmuComment *> *model=new DanmuViewModel<DanmuComment *>(danmuList,this);
    DanmuViewProxyModel *proxyModel = new DanmuViewProxyModel(this);
    proxyModel->setSourceId(sourceId);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setSourceModel(model);
    danmuView->setModel(proxyModel);
    tipLabel->setText(tr("Danmu Count: %1").arg(proxyModel->rowCount()));
    QObject::connect(filterEdit,&DanmuFilterBox::filterChanged,[proxyModel,this](int type, const QString &keyword){
        proxyModel->setFilterKeyColumn(type);
        proxyModel->setFilterFixedString(keyword);
        tipLabel->setText(tr("Danmu Count: %1").arg(proxyModel->rowCount()));
    });
}

DanmuView::DanmuView(const QVector<QSharedPointer<DanmuComment> > *danmuList, QWidget *parent, int sourceId):CFramelessDialog (tr("View Danmu"),parent)
{
    initView();
    DanmuViewModel<QSharedPointer<DanmuComment> > *model=new DanmuViewModel<QSharedPointer<DanmuComment> >(danmuList,this);
    DanmuViewProxyModel *proxyModel = new DanmuViewProxyModel(this);
    proxyModel->setSourceId(sourceId);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(4);
    proxyModel->setSourceModel(model);
    danmuView->setModel(proxyModel);
    tipLabel->setText(tr("Danmu Count: %1").arg(proxyModel->rowCount()));
    QObject::connect(filterEdit,&DanmuFilterBox::filterChanged,[proxyModel,this](int type, const QString &keyword){
        proxyModel->setFilterKeyColumn(type);
        proxyModel->setFilterFixedString(keyword);
        tipLabel->setText(tr("Danmu Count: %1").arg(proxyModel->rowCount()));
    });
}

void DanmuView::initView()
{
    danmuView = new QTreeView(this);
    danmuView->setRootIsDecorated(false);
    danmuView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    danmuView->setItemDelegate(new KTreeviewItemDelegate(this));
    danmuView->setSortingEnabled(true);
    danmuView->header()->setSortIndicator(0, Qt::SortOrder::AscendingOrder);
    danmuView->setObjectName(QStringLiteral("DanmuView"));

    QAction *copy = new QAction(tr("Copy"), this);
    QObject::connect(copy, &QAction::triggered, this, [=](){
        int column = danmuView->columnAt(danmuView->mapFromGlobal(QCursor::pos()).x());
        auto selectedRows= danmuView->selectionModel()->selectedRows(column);
        if(selectedRows.count()==0) return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(selectedRows.first().data(Qt::ToolTipRole).toString());
    });
    danmuView->addAction(copy);
    danmuView->setContextMenuPolicy(Qt::CustomContextMenu);
    ElaMenu *actionMenu = new ElaMenu(danmuView);
    actionMenu->addAction(copy);
    QObject::connect(danmuView, &QTreeView::customContextMenuRequested, this, [=](){
        actionMenu->exec(QCursor::pos());
    });

    tipLabel = new QLabel(this);
    filterEdit=new DanmuFilterBox(this);

    QGridLayout *viewGLayout=new QGridLayout(this);
    viewGLayout->addWidget(tipLabel,0,0);
    viewGLayout->addWidget(filterEdit,0,2);
    viewGLayout->addWidget(danmuView,1,0,1,3);
    viewGLayout->setRowStretch(1,1);
    viewGLayout->setColumnStretch(1,1);
    setSizeSettingKey("DialogSize/DanmuView",QSize(600, 400));
}

DanmuFilterBox::DanmuFilterBox(QWidget *parent): KLineEdit(parent)
  , filterTypeGroup(new QActionGroup(this))
{
    setObjectName(QStringLiteral("FilterEdit"));
    setClearButtonEnabled(true);
    setFont(QFont(GlobalObjects::normalFont, 14));

    QMenu *menu = new ElaMenu(this);

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
    GlobalObjects::iconfont->setPointSize(14);
    optionsButton->setFont(*GlobalObjects::iconfont);
    optionsButton->setText(QChar(0xea8a));
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
