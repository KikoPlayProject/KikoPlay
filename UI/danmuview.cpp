#include "danmuview.h"
#include <QTreeView>
#include <QLabel>
#include <QLineEdit>
#include <QGridLayout>
#include "globalobjects.h"
#include "Play/Danmu/danmuviewmodel.h"
DanmuView::DanmuView(const QList<DanmuComment *> *danmuList, QWidget *parent):CFramelessDialog (tr("View Danmu"),parent)
{
    initView(danmuList->count());
    DanmuViewModel<DanmuComment *> *model=new DanmuViewModel<DanmuComment *>(danmuList,this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(4);
    proxyModel->setSourceModel(model);
    danmuView->setModel(proxyModel);
    QObject::connect(filterEdit,&QLineEdit::textChanged,[proxyModel](const QString &keyword){
        proxyModel->setFilterRegExp(keyword);
    });
}

DanmuView::DanmuView(const QList<QSharedPointer<DanmuComment> > *danmuList, QWidget *parent):CFramelessDialog (tr("View Danmu"),parent)
{
    initView(danmuList->count());
    DanmuViewModel<QSharedPointer<DanmuComment> > *model=new DanmuViewModel<QSharedPointer<DanmuComment> >(danmuList,this);
    QSortFilterProxyModel *proxyModel = new QSortFilterProxyModel(this);
    proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxyModel->setFilterKeyColumn(4);
    proxyModel->setSourceModel(model);
    danmuView->setModel(proxyModel);
    QObject::connect(filterEdit,&QLineEdit::textChanged,[proxyModel](const QString &keyword){
        proxyModel->setFilterRegExp(keyword);
    });
}

void DanmuView::initView(int danmuCount)
{
    danmuView=new QTreeView(this);
    danmuView->setRootIsDecorated(false);
    danmuView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    danmuView->setAlternatingRowColors(true);
    danmuView->setSortingEnabled(true);

    QLabel *tipLabel = new QLabel(tr("Danmu Count: %1").arg(danmuCount),this);
    filterEdit=new QLineEdit(this);

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
