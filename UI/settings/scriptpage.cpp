#include "scriptpage.h"
#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QMessageBox>
#include <QHeaderView>
#include "Download/Script/scriptmanager.h"
#include "globalobjects.h"
ScriptPage::ScriptPage(QWidget *parent) : SettingPage(parent)
{
    QTreeView *scriptView=new QTreeView(this);
    scriptView->setRootIsDecorated(false);
    scriptView->setSelectionMode(QAbstractItemView::SingleSelection);
    scriptView->setModel(GlobalObjects::scriptManager);
    scriptView->setAlternatingRowColors(true);
    QObject::connect(scriptView, &QTreeView::doubleClicked,GlobalObjects::scriptManager,&ScriptManager::setNormalScript);
    scriptView->setColumnWidth(1, 60*logicalDpiX()/96);
    scriptView->setColumnWidth(2, 60*logicalDpiX()/96);

    QPushButton *refresh=new QPushButton(tr("Refresh"),this);
    QPushButton *remove=new QPushButton(tr("Remove"),this);
    QObject::connect(remove,&QPushButton::clicked,[scriptView,this](){
        auto selection = scriptView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        if(QMessageBox::information(this,tr("Remove"),tr("Delete the Script File?"),
                      QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes)
            GlobalObjects::scriptManager->removeScript(selection.last());
    });
    QObject::connect(refresh,&QPushButton::clicked,this,[this,remove,scriptView](){
        emit showBusyState(true);
        remove->setEnabled(false);
        scriptView->setEnabled(false);
        GlobalObjects::scriptManager->refresh();
    });
    QObject::connect(GlobalObjects::scriptManager,&ScriptManager::refreshDone,this,[this,remove,scriptView](){
        emit showBusyState(false);
        remove->setEnabled(true);
        scriptView->setEnabled(true);
    });
    QLabel *tip=new QLabel(QString("<a href = \"https://github.com/Protostars/KikoPlayScript\">%1</a>").arg(tr("More")),this);
    tip->setOpenExternalLinks(true);

    QGridLayout *scriptGLayout=new QGridLayout(this);
    scriptGLayout->addWidget(refresh,0,0);
    scriptGLayout->addWidget(remove,0,1);
    scriptGLayout->addWidget(tip,0,3);
    scriptGLayout->addWidget(scriptView,1,0,1,4);
    scriptGLayout->setRowStretch(1,1);
    scriptGLayout->setColumnStretch(2,1);
    scriptGLayout->setContentsMargins(0, 0, 0, 0);
}

void ScriptPage::onAccept()
{

}

void ScriptPage::onClose()
{

}
