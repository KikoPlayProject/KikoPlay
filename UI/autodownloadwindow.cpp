#include "autodownloadwindow.h"
#include "widgets/fonticonbutton.h"
#include "globalobjects.h"
#include "Download/autodownloadmanager.h"
#include <QAction>
#include <QTreeView>
#include <QGridLayout>
#include <QPushButton>
#include <QButtonGroup>
#include <QStackedLayout>
#include <QSplitter>
#include <QHeaderView>
#include <QApplication>
#include <QClipboard>
#include <QToolButton>
#include "addrule.h"
AutoDownloadWindow::AutoDownloadWindow(QWidget *parent) : QWidget(parent)
{
    FontIconButton *addRuleBtn=new FontIconButton(QChar(0xe600),tr("Add Rule"),12,10,2*logicalDpiX()/96,this);
    addRuleBtn->setObjectName(QStringLiteral("DownloadToolButton"));
    QObject::connect(addRuleBtn, &FontIconButton::clicked, this, [this](){
        AddRule addRule(nullptr, this);
        QRect geo(0,0,300,300);
        geo.moveCenter(this->geometry().center());
        addRule.move(geo.topLeft());
        if(QDialog::Accepted==addRule.exec())
        {
            DownloadRule *rule=addRule.curRule;
            GlobalObjects::autoDownloadManager->addRule(rule);
        }
    });

    QHBoxLayout *toolBarHLayout=new QHBoxLayout();
    toolBarHLayout->setContentsMargins(0,0,0,0);
    toolBarHLayout->addWidget(addRuleBtn);
    toolBarHLayout->addStretch(1);

    ruleView = new QTreeView(this);
    ruleView->setObjectName(QStringLiteral("RuleListView"));
    ruleView->setProperty("cScrollStyle", true);
    ruleView->setRootIsDecorated(false);
    ruleView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ruleView->setAlternatingRowColors(true);
    ruleView->setFont(QFont(GlobalObjects::normalFont,10));
    ruleView->setModel(GlobalObjects::autoDownloadManager);
    ruleView->header()->resizeSection(1, 200 * logicalDpiX() / 96);
    ruleView->header()->resizeSection(2, 160 * logicalDpiX() / 96);
    ruleView->header()->resizeSection(3, 160 * logicalDpiX() / 96);
    QObject::connect(GlobalObjects::autoDownloadManager, &AutoDownloadManager::addTask, this, [this](const QString &uri, const QString &path){
        emit this->addTask(QStringList({uri}), true, path);
    });
    ruleView->setContextMenuPolicy(Qt::ActionsContextMenu);

    logView = new QTreeView(this);
    logView->setRootIsDecorated(false);
    logView->setProperty("cScrollStyle", true);
    logView->setSelectionMode(QAbstractItemView::SingleSelection);
    logView->setAlternatingRowColors(true);
    logView->setFont(QFont(GlobalObjects::normalFont,10));
    logView->setObjectName(QStringLiteral("RuleLogView"));
    LogFilterProxyModel *logProxyModel=new LogFilterProxyModel(this);
    logProxyModel->setSourceModel(GlobalObjects::autoDownloadManager->logModel);
    logView->setModel(logProxyModel);
    logView->header()->resizeSection(0, 200 * logicalDpiX() / 96);
    logView->header()->resizeSection(1, 240 * logicalDpiX() / 96);
    logView->setContextMenuPolicy(Qt::ActionsContextMenu);
    QObject::connect(GlobalObjects::autoDownloadManager->logModel, &LogModel::rowsInserted,logView, &QTreeView::scrollToBottom);

    urlView = new QTreeView(this);
    urlView->setRootIsDecorated(false);
    urlView->setProperty("cScrollStyle", true);
    urlView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    urlView->setAlternatingRowColors(true);
    urlView->setFont(QFont(GlobalObjects::normalFont,10));
    urlView->setObjectName(QStringLiteral("RuleURLView"));
    urlView->setModel(GlobalObjects::autoDownloadManager->urlModel);
    urlView->header()->resizeSection(0, 500 * logicalDpiX() / 96);
    urlView->setContextMenuPolicy(Qt::ActionsContextMenu);
    QObject::connect(GlobalObjects::autoDownloadManager->urlModel, &URLModel::rowsInserted,urlView, &QTreeView::scrollToBottom);

    int pageBtnHeight=28*logicalDpiY()/96;

    QToolButton *logPage=new QToolButton(this);
    logPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    logPage->setText(tr("Log"));
    logPage->setFixedHeight(pageBtnHeight);
    logPage->setCheckable(true);

    QToolButton *urlPage=new QToolButton(this);
    urlPage->setObjectName(QStringLiteral("DownloadInfoPage"));
    urlPage->setFixedHeight(pageBtnHeight);
    urlPage->setText(tr("Staging URI"));
    urlPage->setCheckable(true);

    QHBoxLayout *pageBarHLayout=new QHBoxLayout();
    pageBarHLayout->setContentsMargins(0,0,0,2*logicalDpiY()/96);
    pageBarHLayout->addWidget(logPage);
    pageBarHLayout->addWidget(urlPage);
    pageBarHLayout->addStretch(1);

    QButtonGroup *pageButtonGroup=new QButtonGroup(this);
    pageButtonGroup->addButton(logPage,0);
    pageButtonGroup->addButton(urlPage,1);
    logPage->setChecked(true);

    QWidget *logContent=new QWidget(this);
    logContent->setContentsMargins(0,0,0,0);
    QStackedLayout *logSLayout=new QStackedLayout(logContent);
    logSLayout->addWidget(logView);
    logSLayout->addWidget(urlView);
    QObject::connect(pageButtonGroup,(void (QButtonGroup:: *)(int, bool))&QButtonGroup::buttonToggled,[logSLayout](int id, bool checked){
        if(checked)logSLayout->setCurrentIndex(id);
    });

    QWidget *bottomContent=new QWidget(this);
    QVBoxLayout *bvLayout=new QVBoxLayout(bottomContent);
    bvLayout->setContentsMargins(0,0,0,0);
    bvLayout->addLayout(pageBarHLayout);
    bvLayout->addWidget(logContent);

    QSplitter *viewBottomSplitter=new QSplitter(Qt::Vertical,this);
    viewBottomSplitter->setObjectName(QStringLiteral("NormalSplitter"));
    viewBottomSplitter->addWidget(ruleView);
    viewBottomSplitter->addWidget(bottomContent);
    viewBottomSplitter->setStretchFactor(0,1);
    viewBottomSplitter->setStretchFactor(1,1);
    viewBottomSplitter->setCollapsible(0,false);
    viewBottomSplitter->setCollapsible(1,true);

    QGridLayout *downContainerGLayout=new QGridLayout(this);
    downContainerGLayout->addLayout(toolBarHLayout,0,1);
    downContainerGLayout->addWidget(viewBottomSplitter,1,1);
    downContainerGLayout->setRowStretch(1,1);

    setupActions();
}

void AutoDownloadWindow::setupActions()
{
    QAction *enbaleRule=new QAction(tr("Enable Rule(s)"), this);
    QObject::connect(enbaleRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if(selectedRows.size()==0) return;
        GlobalObjects::autoDownloadManager->startRules(selectedRows);
    });
    QAction *disableRule=new QAction(tr("Disable Rule(s)"), this);
    QObject::connect(disableRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if(selectedRows.size()==0) return;
        GlobalObjects::autoDownloadManager->stopRules(selectedRows);
    });
    QAction *checkRule=new QAction(tr("Check Immediately)"), this);
    QObject::connect(checkRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if(selectedRows.size()==0) return;
        GlobalObjects::autoDownloadManager->checkAtOnce(selectedRows);
    });
    QAction *editRule=new QAction(tr("Edit"), this);
    QObject::connect(editRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if(selectedRows.size()==0) return;
        DownloadRule *rule = GlobalObjects::autoDownloadManager->getRule(selectedRows.last(), true);
        if(!rule) return;
        AddRule addRule(rule, this);
        QRect geo(0,0,300,300);
        geo.moveCenter(this->geometry().center());
        addRule.move(geo.topLeft());
        if(QDialog::Accepted==addRule.exec())
            GlobalObjects::autoDownloadManager->applyRule(rule, true);
        else
            GlobalObjects::autoDownloadManager->applyRule(rule, false);
    });
    QAction *removeRule=new QAction(tr("Remove Rule(s)"), this);
    QObject::connect(removeRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if(selectedRows.size()==0) return;
        GlobalObjects::autoDownloadManager->removeRules(selectedRows);
    });
    ruleView->addAction(enbaleRule);
    ruleView->addAction(disableRule);
    ruleView->addAction(checkRule);
    ruleView->addAction(editRule);
    ruleView->addAction(removeRule);

    QObject::connect(ruleView, &QTreeView::doubleClicked,[](const QModelIndex &index){
        DownloadRule *rule = GlobalObjects::autoDownloadManager->getRule(index);
        if(rule->state==0)
            GlobalObjects::autoDownloadManager->stopRules(QModelIndexList()<<index);
        else if(rule->state==2)
            GlobalObjects::autoDownloadManager->startRules(QModelIndexList()<<index);
    });
    QObject::connect(ruleView->selectionModel(), &QItemSelectionModel::selectionChanged,this,[this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if(selectedRows.size()==0)
        {
            static_cast<LogFilterProxyModel *>(logView->model())->setFilterId(-1);
            GlobalObjects::autoDownloadManager->urlModel->setRule(nullptr);            
        }
        else
        {
            DownloadRule *rule = GlobalObjects::autoDownloadManager->getRule(selectedRows.last());
            static_cast<LogFilterProxyModel *>(logView->model())->setFilterId(rule->id);
            GlobalObjects::autoDownloadManager->urlModel->setRule(rule);
        }
        logView->scrollToBottom();
        urlView->scrollToBottom();
    });
    QAction *copyLog=new QAction(tr("Copy"), this);
    QObject::connect(copyLog, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows(logView->selectionModel()->selectedRows());
        if(selectedRows.size()==0)return;
        QModelIndex index = static_cast<LogFilterProxyModel *>(logView->model())->mapToSource(selectedRows.last());
        QClipboard *cb = QApplication::clipboard();
        cb->setText(GlobalObjects::autoDownloadManager->logModel->getLog(index));
    });
    logView->addAction(copyLog);

    QAction *addToDownload=new QAction(tr("Add URI Task"), this);
    QObject::connect(addToDownload, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= urlView->selectionModel()->selectedRows();
        if(selectedRows.size()==0) return;
        QStringList uris(GlobalObjects::autoDownloadManager->urlModel->getSelectedURIs(selectedRows));
        emit this->addTask(uris, false, GlobalObjects::autoDownloadManager->urlModel->getRule()->filePath);
    });
    QAction *copyURIs=new QAction(tr("Copy URI"), this);
    QObject::connect(copyURIs, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= urlView->selectionModel()->selectedRows();
        if(selectedRows.size()==0) return;
        QStringList uris(GlobalObjects::autoDownloadManager->urlModel->getSelectedURIs(selectedRows));
        QClipboard *cb = QApplication::clipboard();
        cb->setText(uris.join('\n'));
    });
    QAction *removeURIs=new QAction(tr("Remove"), this);
    QObject::connect(removeURIs, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= urlView->selectionModel()->selectedRows();
        if(selectedRows.size()==0) return;
        GlobalObjects::autoDownloadManager->urlModel->removeSelectedURIs(selectedRows);
    });
    urlView->addAction(addToDownload);
    urlView->addAction(copyURIs);
    urlView->addAction(removeURIs);
}
