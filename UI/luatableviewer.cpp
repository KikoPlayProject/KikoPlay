#include "luatableviewer.h"
#include <QTreeView>
#include <QHeaderView>
#include <QAction>
#include <QGridLayout>
#include <QClipboard>
#include <QApplication>
#include <QSettings>
#include <QSortFilterProxyModel>
#include "Extension/Common/luatablemodel.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "globalobjects.h"

LuaTableViewer::LuaTableViewer(LuaTableModel *model, QWidget *parent) :
    CFramelessDialog(tr("Lua Table Viewer"), parent), currentColumn(0)
{
    QTreeView *view = new QTreeView(this);
    view->setAlternatingRowColors(true);
    view->setAnimated(true);
    view->setItemDelegate(new KTreeviewItemDelegate(view));
    QSortFilterProxyModel *proxyModel=new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(model);
    view->setModel(proxyModel);
    view->setSortingEnabled(true);

    QAction *copy = new QAction(tr("Copy"), this);
    QObject::connect(copy, &QAction::triggered, this, [=](){
        int column = view->columnAt(view->mapFromGlobal(QCursor::pos()).x());
        auto selectedRows= view->selectionModel()->selectedRows(column);
        if(selectedRows.count()==0) return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(selectedRows.first().data(Qt::ToolTipRole).toString());
    });
    view->addAction(copy);
    view->setContextMenuPolicy(Qt::CustomContextMenu);
    ElaMenu *actionMenu = new ElaMenu(view);
    actionMenu->addAction(copy);
    QObject::connect(view, &QTreeView::customContextMenuRequested, this, [=](){
        actionMenu->exec(QCursor::pos());
    });

    QVariant headerState(GlobalObjects::appSetting->value("HeaderViewState/LuaTableView"));
    if(!headerState.isNull())
        view->header()->restoreState(headerState.toByteArray());

    addOnCloseCallback([=](){
        GlobalObjects::appSetting->setValue("HeaderViewState/LuaTableView", view->header()->saveState());
    });

    QGridLayout *viewGLayout = new QGridLayout(this);
    viewGLayout->addWidget(view, 0, 0);
    viewGLayout->setContentsMargins(0, 0, 0, 0);

    setSizeSettingKey("DialogSize/LuaTabelViewer",QSize(300*logicalDpiX()/96, 400*logicalDpiY()/96));

}
