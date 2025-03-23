#include "scriptpage.h"
#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QAction>
#include <QMenu>
#include <QHeaderView>
#include "Common/threadtask.h"
#include "Extension/Script/scriptmodel.h"
#include "Extension/Script/scriptsettingmodel.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/kpushbutton.h"
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
    new FloatScrollBar(scriptView->verticalScrollBar(), scriptView);


    QComboBox *typeCombo = new ElaComboBox(this);
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
    QObject::connect(scriptView, &QTreeView::doubleClicked,[=](const QModelIndex &sindex){
        if(!sindex.isValid()) return;
        auto index = proxyModel->mapToSource(sindex).siblingAtColumn((int)ScriptModel::Columns::ID);
        auto s = GlobalObjects::scriptManager->getScript(index.data().toString());
        if(s->settings().size()==0) return;
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

    QMenu *scriptViewContextMenu = new ElaMenu(scriptView);
    scriptViewContextMenu->addAction(actSetting);
    scriptViewContextMenu->addAction(actRemove);
    QAction *menuSep = new QAction(this);
    menuSep->setSeparator(true);
    static QVector<QAction *> scriptActions;
    QObject::connect(scriptView, &QTreeView::customContextMenuRequested,[=](){
        auto selection = scriptView->selectionModel()->selectedRows((int)ScriptModel::Columns::ID);
        for(QAction *act : scriptActions)
            scriptViewContextMenu->removeAction(act);
        scriptViewContextMenu->removeAction(menuSep);
        qDeleteAll(scriptActions);
        scriptActions.clear();
        if(selection.size()>0)
        {
            auto index = proxyModel->mapToSource(selection.last());
            auto script = GlobalObjects::scriptManager->getScript(index.data().toString());
            if(script)
            {
                const auto &menuItems = script->getScriptMenuItems();
                if(menuItems.size()>0)
                {
                    scriptViewContextMenu->addAction(menuSep);
                    for(auto &p : menuItems)
                    {
                        QAction *scriptAct = new QAction(p.first);
                        scriptAct->setData(p.second);
                        scriptViewContextMenu->addAction(scriptAct);
                        scriptActions.append(scriptAct);
                    }
                }
            }

        }
        scriptViewContextMenu->exec(QCursor::pos());
    });
    QObject::connect(scriptViewContextMenu, &QMenu::triggered, this, [=](QAction *act){
        if(!act->data().isNull())
        {
            auto selection = scriptView->selectionModel()->selectedRows((int)ScriptModel::Columns::ID);
            if (selection.size() == 0)return;
            auto index = proxyModel->mapToSource(selection.last());
            auto s = GlobalObjects::scriptManager->getScript(index.data().toString());
            s->scriptMenuClick(act->data().toString());
        }
    });

    scriptView->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

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

    QPushButton *refresh = new KPushButton(tr("Refresh"), this);
    QObject::connect(refresh, &QPushButton::clicked, this, [=](){
        emit showBusyState(true);
        actRemove->setEnabled(false);
        actSetting->setEnabled(false);
        scriptView->setEnabled(false);
        typeCombo->setEnabled(false);
        ThreadTask task(GlobalObjects::workThread);
        int type = typeCombo->currentIndex();
        task.Run([type](){
            if (type<ScriptType::UNKNOWN_STYPE)
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

    QLabel *tip=new QLabel(QString("<a style='color: rgb(96, 208, 252);' href = \"https://github.com/KikoPlayProject/KikoPlayScript\">%1</a>").arg(tr("More")),this);
    tip->setOpenExternalLinks(true);

    QGridLayout *scriptGLayout=new QGridLayout(this);
    scriptGLayout->addWidget(typeCombo,0,0);
    scriptGLayout->addWidget(refresh,0,1);
    scriptGLayout->addWidget(tip,0,3);
    scriptGLayout->addWidget(scriptView,1,0,1,4);
    scriptGLayout->setRowStretch(1,1);
    scriptGLayout->setColumnStretch(2,1);
}

ScriptSettingDialog::ScriptSettingDialog(QSharedPointer<ScriptBase> script, QWidget *parent) : CFramelessDialog(tr("Script Settings"), parent, true)
{
    ScriptSettingModel *model = new ScriptSettingModel(script, this);
    SettingDelegate *delegate = new SettingDelegate(this);


    QTreeView *scriptView=new QTreeView(this);
    scriptView->setObjectName(QStringLiteral("ScriptSettingView"));
    scriptView->setRootIsDecorated(false);
    scriptView->setItemDelegate(delegate);
    scriptView->setSelectionMode(QAbstractItemView::SingleSelection);
    scriptView->setModel(model);
    scriptView->setAlternatingRowColors(true);
    scriptView->expandAll();
    new FloatScrollBar(scriptView->verticalScrollBar(), scriptView);
    QObject::connect(model, &ScriptSettingModel::itemChanged, this, [=](const QString &, int index, const QString &value){
       changedItems[index] = value;
    });

    QGridLayout *scriptGLayout=new QGridLayout(this);
    scriptGLayout->addWidget(scriptView,0,0);
    scriptGLayout->setRowStretch(0,1);
    scriptGLayout->setColumnStretch(0,1);


    QVariant headerState(GlobalObjects::appSetting->value("HeaderViewState/ScriptSettingView"));
    if(!headerState.isNull())
        scriptView->header()->restoreState(headerState.toByteArray());

    addOnCloseCallback([scriptView](){
        GlobalObjects::appSetting->setValue("HeaderViewState/ScriptSettingView", scriptView->header()->saveState());
    });

    setSizeSettingKey("DialogSize/ScriptSetting", QSize(300, 200));
}

