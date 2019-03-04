#include "managescript.h"
#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QMessageBox>
#include "Download/Script/scriptmanager.h"
ManageScript::ManageScript(ScriptManager *manager, QWidget *parent):CFramelessDialog(tr("Manage Script"),parent)
{
    QTreeView *scriptView=new QTreeView(this);
    scriptView->setRootIsDecorated(false);
    scriptView->setSelectionMode(QAbstractItemView::SingleSelection);
    scriptView->setModel(manager);
    scriptView->setAlternatingRowColors(true);
    QObject::connect(scriptView, &QTreeView::doubleClicked,manager,&ScriptManager::setNormalScript);

    QPushButton *refresh=new QPushButton(tr("Refresh"),this);
    QPushButton *remove=new QPushButton(tr("Remove"),this);
    QObject::connect(remove,&QPushButton::clicked,[scriptView,manager,this](){
        auto selection = scriptView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        if(QMessageBox::information(this,tr("Remove"),tr("Delete the Script File?"),
                      QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes)
            manager->removeScript(selection.last());
    });
    QObject::connect(refresh,&QPushButton::clicked,this,[this,manager,remove,scriptView](){
        showBusyState(true);
        remove->setEnabled(false);
        scriptView->setEnabled(false);
        manager->refresh();
    });
    QObject::connect(manager,&ScriptManager::refreshDone,this,[this,remove,scriptView](){
        showBusyState(false);
        remove->setEnabled(true);
        scriptView->setEnabled(true);
    });
    QGridLayout *scriptGLayout=new QGridLayout(this);
    scriptGLayout->addWidget(refresh,0,0);
    scriptGLayout->addWidget(remove,0,1);
    scriptGLayout->addWidget(scriptView,1,0,1,3);
    scriptGLayout->setRowStretch(1,1);
    scriptGLayout->setColumnStretch(2,1);
    scriptGLayout->setContentsMargins(0, 0, 0, 0);
    resize(QSize(400*logicalDpiX()/96,300*logicalDpiY()/96));

}
