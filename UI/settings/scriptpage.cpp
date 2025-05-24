#include "scriptpage.h"
#include <QTreeView>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QMessageBox>
#include <QComboBox>
#include <QAction>
#include <QMenu>
#include <QScrollArea>
#include <QHeaderView>
#include <QMouseEvent>
#include "Common/threadtask.h"
#include "Extension/Script/scriptmodel.h"
#include "UI/ela/ElaComboBox.h"
#include "UI/ela/ElaLineEdit.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/kpushbutton.h"
#include "globalobjects.h"

namespace
{
    class ScriptView : public QTreeView
    {
    public:
        ScriptView(QWidget* parent = nullptr) : QTreeView(parent)  { }
    protected:
        void mouseMoveEvent(QMouseEvent* event) override
        {
            QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
            QModelIndex index = indexAt(mouseEvent->pos());
            if (index.isValid() && index.column() == (int)ScriptModel::Columns::OPERATE)
            {
                update(index);
            }
            QTreeView::mouseMoveEvent(event);
        }
    };
}
ScriptPage::ScriptPage(QWidget *parent) : SettingPage(parent)
{
    ScriptModel *model = new ScriptModel(this);
    ScriptProxyModel *proxyModel = new ScriptProxyModel(this);
    proxyModel->setSourceModel(model);
    ScriptItemDelegate *delegate = new ScriptItemDelegate(this);

    QTreeView *scriptView=new ScriptView(this);
    scriptView->setRootIsDecorated(false);
    scriptView->setSelectionMode(QAbstractItemView::SingleSelection);
    scriptView->setModel(proxyModel);
    scriptView->setAlternatingRowColors(true);
    scriptView->setItemDelegate(delegate);
    new FloatScrollBar(scriptView->verticalScrollBar(), scriptView);
    scriptView->hideColumn((int)ScriptModel::Columns::ID);
    scriptView->setMouseTracking(true);


    QComboBox *typeCombo = new ElaComboBox(this);
    typeCombo->addItems(model->scriptTypes);
    typeCombo->addItem(tr("All"));
    typeCombo->setCurrentIndex(model->scriptTypes.size());
    QObject::connect(typeCombo,(void (QComboBox:: *)(int))&QComboBox::currentIndexChanged, this, [=](int index){
        if(index == model->scriptTypes.size()) proxyModel->setType(-1);
        else proxyModel->setType(index);
    });

    QObject::connect(delegate, &ScriptItemDelegate::settingClicked, this, [=](const QModelIndex &index){
        auto sindex = proxyModel->mapToSource(index).siblingAtColumn((int)ScriptModel::Columns::ID);
        auto s = GlobalObjects::scriptManager->getScript(sindex.data().toString());
        ScriptSettingDialog dialog(s, this);
        if (QDialog::Accepted==dialog.exec())
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
    scriptViewContextMenu->addAction(actRemove);
    QObject::connect(scriptView, &QTreeView::customContextMenuRequested, this, [=](){
        if (scriptView->selectionModel()->hasSelection())
        {
            scriptViewContextMenu->exec(QCursor::pos());
        }
    });
    scriptView->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);

    QObject::connect(delegate, &ScriptItemDelegate::menuClicked, this, [=](const QModelIndex &index){
        auto sindex = proxyModel->mapToSource(index).siblingAtColumn((int)ScriptModel::Columns::ID);
        auto script = GlobalObjects::scriptManager->getScript(sindex.data().toString());
        if (script)
        {
            const auto &menuItems = script->getScriptMenuItems();
            if (!menuItems.empty())
            {
                QMenu *scriptMenu = new ElaMenu(scriptView);
                scriptMenu->setAttribute(Qt::WA_DeleteOnClose);
                for(auto &p : menuItems)
                {
                    QAction *scriptAct = scriptMenu->addAction(p.first);
                    scriptAct->setData(p.second);
                }
                QObject::connect(scriptMenu, &QMenu::triggered, this, [=](QAction *act){
                    if (!act->data().isNull())
                    {
                        script->scriptMenuClick(act->data().toString());
                    }
                });
                scriptMenu->exec(QCursor::pos());
            }
        }
    });

    QPushButton *refresh = new KPushButton(tr("Refresh"), this);
    QObject::connect(refresh, &QPushButton::clicked, this, [=](){
        emit showBusyState(true);
        actRemove->setEnabled(false);
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
    const QVector<ScriptBase::ScriptSettingItem> &settings = script->settings();
    QList<QPair<QString, QList<QPair<const ScriptBase::ScriptSettingItem *, int>>>> groupItems;
    QMap<QString, int> groupIndexMap;
    QList<QPair<const ScriptBase::ScriptSettingItem *, int>> nonGroupItems;
    int itemIndex = 0;
    for (const ScriptBase::ScriptSettingItem &item : settings)
    {
        if(!item.group.isEmpty())
        {
            int groupIndex = groupIndexMap.value(item.group, -1);
            if (groupIndex < 0)
            {
                groupIndexMap[item.group] = groupItems.size();
                groupItems.push_back({item.group, {}});
                groupIndex = groupIndexMap[item.group];
            }
            groupItems[groupIndex].second.append({&item, itemIndex++});
        }
        else
        {
            nonGroupItems.append({&item, itemIndex++});
        }
    }

    auto addSettingArea = [this](const QString &title, const QList<QPair<const ScriptBase::ScriptSettingItem *, int>> &items){
        SettingItemArea *itemArea = new SettingItemArea(title, this);
        for (auto &p : items)
        {
            QWidget *editor{nullptr};
            int setttingIndex = p.second;
            if (p.first->choices.isEmpty())
            {
                ElaLineEdit *lineEditor = new ElaLineEdit(p.first->value, itemArea);
                QObject::connect(lineEditor, &QLineEdit::textEdited, this, [=](const QString &text){
                    changedItems[setttingIndex] = text;
                });
                editor = lineEditor;
            }
            else
            {
                ElaComboBox *combo = new ElaComboBox(itemArea);
                QStringList items = p.first->choices.split(',', Qt::SkipEmptyParts);
                combo->addItems(items);
                combo->setCurrentIndex(items.indexOf(p.first->value));
                QObject::connect(combo, &QComboBox::currentTextChanged, this, [=](const QString &text){
                    changedItems[setttingIndex] = text;
                });
                editor = combo;
            }
            itemArea->addItem(p.first->title, editor);
            if (!p.first->description.isEmpty())
            {
                QLabel *descLabel = new QLabel(p.first->description, itemArea);
                descLabel->setObjectName(QStringLiteral("SettingDescLabel"));
                itemArea->addItem(descLabel, Qt::AlignLeft);
            }
        }
        return itemArea;
    };

    QWidget *container = new QWidget(this);
    QVBoxLayout *itemVLayout = new QVBoxLayout(container);
    itemVLayout->setSpacing(8);
    if (!nonGroupItems.empty()) itemVLayout->addWidget(addSettingArea(tr("Default"), nonGroupItems));
    for (auto &p : groupItems)
    {
        itemVLayout->addWidget(addSettingArea(p.first, p.second));
    }
    itemVLayout->addStretch(1);

    QScrollArea *pageScrollArea = new QScrollArea(this);
    pageScrollArea->setWidget(container);
    pageScrollArea->setWidgetResizable(true);
    new FloatScrollBar(pageScrollArea->verticalScrollBar(), pageScrollArea);
    QHBoxLayout *sHLayout = new QHBoxLayout(this);
    sHLayout->setContentsMargins(0, 0, 0, 0);
    sHLayout->addWidget(pageScrollArea);

    setSizeSettingKey(QString("DialogSize/ScriptSetting_%1").arg(script->id()), QSize(400, 400));
}

