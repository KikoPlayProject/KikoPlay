#include "scriptpage.h"
#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QAction>
#include <QHeaderView>
#include "Common/threadtask.h"
#include "Script/scriptmodel.h"
#include "Script/scriptsettingmodel.h"
#include "globalobjects.h"
ScriptPage::ScriptPage(QWidget *parent) : SettingPage(parent)
{
    ScriptModel *model = new ScriptModel(this);
    ScriptProxyModel *proxyModel = new ScriptProxyModel(this);
    proxyModel->setSourceModel(model);

    QTreeView *scriptView=new QTreeView(this);
    scriptView->setRootIsDecorated(false);
    scriptView->setSelectionMode(QAbstractItemView::SingleSelection);
    scriptView->setModel(proxyModel);
    scriptView->setAlternatingRowColors(true);
    //scriptView->setColumnWidth(1, 60*logicalDpiX()/96);
    //scriptView->setColumnWidth(2, 60*logicalDpiX()/96);

    QComboBox *typeCombo = new QComboBox(this);
    typeCombo->addItems(model->scriptTypes);
    typeCombo->addItem(tr("All"));
    typeCombo->setCurrentIndex(model->scriptTypes.size());
    QObject::connect(typeCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, this, [=](int index){
        if(index == model->scriptTypes.size()) proxyModel->setType(-1);
        else proxyModel->setType(index);
    });

    QAction *actSetting = new QAction(tr("Script Settings"), this);
    QObject::connect(actSetting, &QAction::triggered, this, [=](){
        auto selection = scriptView->selectionModel()->selectedRows((int)ScriptModel::Columns::ID);
        if (selection.size() == 0)return;
        auto index = proxyModel->mapToSource(selection.last());
        auto s = GlobalObjects::scriptManager->getScript(index.data().toString());
        ScriptSettingDialog dialog(s, this);
        if(QDialog::Accepted==dialog.exec())
        {
            for(auto iter = dialog.changedItems.begin(); iter!=dialog.changedItems.end();++iter)
            {
                s->setOption(iter.key(), iter.value());
            }
        }
    });
    QAction *actRemove = new QAction(tr("Remove"), this);
    QObject::connect(actRemove, &QAction::triggered, this, [=](){
        auto selection = scriptView->selectionModel()->selectedRows((int)ScriptModel::Columns::ID);
        if (selection.size() == 0)return;
        if(QMessageBox::information(this,tr("Remove"),tr("Delete the Script File?"),
                      QMessageBox::Yes|QMessageBox::No,QMessageBox::No)==QMessageBox::Yes)
        {
            auto index = proxyModel->mapToSource(selection.last());
            GlobalObjects::scriptManager->deleteScript(index.data().toString());
        }
    });
    scriptView->setContextMenuPolicy(Qt::ContextMenuPolicy::ActionsContextMenu);
    scriptView->addAction(actSetting);
    scriptView->addAction(actRemove);
    QObject::connect(scriptView->selectionModel(), &QItemSelectionModel::selectionChanged,this, [=](){
        auto selection = scriptView->selectionModel()->selectedRows((int)ScriptModel::Columns::ID);
        if (selection.size() == 0)
        {
            actRemove->setEnabled(false);
            actSetting->setEnabled(false);
        }
        else
        {
            actRemove->setEnabled(true);
            auto index = proxyModel->mapToSource(selection.last());
            auto s = GlobalObjects::scriptManager->getScript(index.data().toString());
            actSetting->setEnabled(s->settings().size()>0);
        }
    });

    QPushButton *refresh=new QPushButton(tr("Refresh"),this);
    QObject::connect(refresh,&QPushButton::clicked,this,[=](){
        emit showBusyState(true);
        actRemove->setEnabled(false);
        actSetting->setEnabled(false);
        scriptView->setEnabled(false);
        typeCombo->setEnabled(false);
        ThreadTask task(GlobalObjects::workThread);
        int type = typeCombo->currentIndex();
        task.Run([type](){
            if(type<ScriptType::UNKNOWN_STYPE)
            {
                GlobalObjects::scriptManager->refreshScripts(ScriptType(type));
            }
            else
            {
                for(int i=ScriptType::DANMU; i<ScriptType::UNKNOWN_STYPE;++i)
                    GlobalObjects::scriptManager->refreshScripts(ScriptType(i));
            }
            return 0;
        });
        emit showBusyState(false);
        actRemove->setEnabled(true);
        actSetting->setEnabled(true);
        scriptView->setEnabled(true);
        typeCombo->setEnabled(true);
    });

    QLabel *tip=new QLabel(QString("<a href = \"https://github.com/Protostars/KikoPlayScript\">%1</a>").arg(tr("More")),this);
    tip->setOpenExternalLinks(true);

    QGridLayout *scriptGLayout=new QGridLayout(this);
    scriptGLayout->addWidget(typeCombo,0,0);
    scriptGLayout->addWidget(refresh,0,1);
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

ScriptSettingDialog::ScriptSettingDialog(QSharedPointer<ScriptBase> script, QWidget *parent) : CFramelessDialog(tr("Script Settings"), parent, true)
{
    ScriptSettingModel *model = new ScriptSettingModel(script, this);
    SettingDelegate *delegate = new SettingDelegate(this);

    QTreeView *scriptView=new QTreeView(this);
    scriptView->setRootIsDecorated(false);
    scriptView->setItemDelegate(delegate);
    scriptView->setSelectionMode(QAbstractItemView::SingleSelection);
    scriptView->setModel(model);
    scriptView->setAlternatingRowColors(true);

    QObject::connect(model, &ScriptSettingModel::itemChanged, this, [=](const QString &, int index, const QString &value){
       changedItems[index] = value;
    });

    QGridLayout *scriptGLayout=new QGridLayout(this);
    scriptGLayout->addWidget(scriptView,0,0);
    scriptGLayout->setRowStretch(0,1);
    scriptGLayout->setColumnStretch(0,1);
}
