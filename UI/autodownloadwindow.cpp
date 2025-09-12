#include "autodownloadwindow.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "UI/widgets/floatscrollbar.h"
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
    FontIconButton *addRuleBtn = new FontIconButton(QChar(0xe600), tr("Add Rule"), 12, 10, 2, this);
    addRuleBtn->setObjectName(QStringLiteral("FontIconToolButton"));
    addRuleBtn->setContentsMargins(4, 2, 4, 2);
    QObject::connect(addRuleBtn, &FontIconButton::clicked, this, [this](){
        AddRule addRule(nullptr, this);
        if (QDialog::Accepted == addRule.exec())
        {
            DownloadRule *rule = addRule.curRule;
            GlobalObjects::autoDownloadManager->addRule(rule);
        }
    });

    QHBoxLayout *toolBarHLayout = new QHBoxLayout();
    toolBarHLayout->setContentsMargins(0,0,0,0);
    toolBarHLayout->addWidget(addRuleBtn);
    toolBarHLayout->addStretch(1);

    ruleView = new QTreeView(this);
    ruleView->setRootIsDecorated(false);
    ruleView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    ruleView->setAlternatingRowColors(true);
    ruleView->setFont(QFont(GlobalObjects::normalFont,11));
    ruleView->setItemDelegate(new KTreeviewItemDelegate(ruleView));
    ruleView->setModel(GlobalObjects::autoDownloadManager);
    new FloatScrollBar(ruleView->verticalScrollBar(), ruleView);
    ruleView->header()->resizeSection(1, 200);
    ruleView->header()->resizeSection(2, 160);
    ruleView->header()->resizeSection(3, 160);
    QObject::connect(GlobalObjects::autoDownloadManager, &AutoDownloadManager::addTask, this, [this](const QString &uri, const QString &path){
        emit this->addTask(QStringList({uri}), true, path);
    });
    ruleView->setContextMenuPolicy(Qt::CustomContextMenu);

    logView = new QTreeView(this);
    logView->setRootIsDecorated(false);
    logView->setSelectionMode(QAbstractItemView::SingleSelection);
    logView->setAlternatingRowColors(true);
    logView->setItemDelegate(new KTreeviewItemDelegate(logView));
    logView->setFont(QFont(GlobalObjects::normalFont, 11));
    new FloatScrollBar(logView->verticalScrollBar(), logView);
    LogFilterProxyModel *logProxyModel=new LogFilterProxyModel(this);
    logProxyModel->setSourceModel(GlobalObjects::autoDownloadManager->logModel);
    logView->setModel(logProxyModel);
    logView->header()->resizeSection(0, 200);
    logView->header()->resizeSection(1, 240);
    logView->header()->setFont(QFont(GlobalObjects::normalFont, 12));
    logView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(GlobalObjects::autoDownloadManager->logModel, &LogModel::rowsInserted,logView, &QTreeView::scrollToBottom);

    urlView = new QTreeView(this);
    urlView->setRootIsDecorated(false);
    urlView->setItemDelegate(new KTreeviewItemDelegate(urlView));
    urlView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    urlView->setAlternatingRowColors(true);
    new FloatScrollBar(urlView->verticalScrollBar(), urlView);
    urlView->setFont(QFont(GlobalObjects::normalFont,10));
    urlView->setModel(GlobalObjects::autoDownloadManager->urlModel);
    urlView->header()->resizeSection(0, 500);
    urlView->header()->setFont(QFont(GlobalObjects::normalFont, 12));
    urlView->setContextMenuPolicy(Qt::CustomContextMenu);
    QObject::connect(GlobalObjects::autoDownloadManager->urlModel, &URLModel::rowsInserted, urlView, &QTreeView::scrollToBottom);

    QToolButton *logPage=new QToolButton(this);
    logPage->setObjectName(QStringLiteral("DownloadInfoPageButton"));
    logPage->setText(tr("Log"));
    logPage->setCheckable(true);

    QToolButton *urlPage=new QToolButton(this);
    urlPage->setObjectName(QStringLiteral("DownloadInfoPageButton"));
    urlPage->setText(tr("Staging URI"));
    urlPage->setCheckable(true);

    QFontMetrics fm(logPage->fontMetrics());
    int pageBtnHeight = fm.height() + 10;
    int pageBtnWidth = qMax(fm.horizontalAdvance(logPage->text()), fm.horizontalAdvance(urlPage->text())) * 1.5;
    logPage->setFixedSize(pageBtnWidth, pageBtnHeight);
    urlPage->setFixedSize(pageBtnWidth, pageBtnHeight);

    QHBoxLayout *pageBarHLayout=new QHBoxLayout();
    pageBarHLayout->setContentsMargins(0,0,0,2);
    pageBarHLayout->setSpacing(2);
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
    QObject::connect(pageButtonGroup,&QButtonGroup::idToggled,[logSLayout](int id, bool checked){
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

    QGridLayout *downContainerGLayout = new QGridLayout(this);
    downContainerGLayout->addLayout(toolBarHLayout,0,1);
    downContainerGLayout->addWidget(viewBottomSplitter,1,1);
    downContainerGLayout->setRowStretch(1,1);

    setupActions();
}

QColor AutoDownloadWindow::getGeneralColor() const
{
    return GlobalObjects::autoDownloadManager->logModel->getLogColor(DownloadRuleLog::LOG_GENERAL);
}

void AutoDownloadWindow::setGeneralColor(const QColor &color)
{
    GlobalObjects::autoDownloadManager->logModel->setLogColor(color, DownloadRuleLog::LOG_GENERAL);
}

QColor AutoDownloadWindow::getResFindedColor() const
{
    return GlobalObjects::autoDownloadManager->logModel->getLogColor(DownloadRuleLog::LOG_RES_FINDED);
}

void AutoDownloadWindow::setResFindedColor(const QColor &color)
{
    GlobalObjects::autoDownloadManager->logModel->setLogColor(color, DownloadRuleLog::LOG_RES_FINDED);
}

QColor AutoDownloadWindow::getErrorColor() const
{
    return GlobalObjects::autoDownloadManager->logModel->getLogColor(DownloadRuleLog::LOG_ERROR);
}

void AutoDownloadWindow::setErrorColor(const QColor &color)
{
    GlobalObjects::autoDownloadManager->logModel->setLogColor(color, DownloadRuleLog::LOG_ERROR);
}

void AutoDownloadWindow::setupActions()
{
    QAction *enbaleRule=new QAction(tr("Enable Rule(s)"), this);
    QObject::connect(enbaleRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        GlobalObjects::autoDownloadManager->startRules(selectedRows);
    });
    QAction *disableRule=new QAction(tr("Disable Rule(s)"), this);
    QObject::connect(disableRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        GlobalObjects::autoDownloadManager->stopRules(selectedRows);
    });
    QAction *checkRule=new QAction(tr("Check Immediately)"), this);
    QObject::connect(checkRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        GlobalObjects::autoDownloadManager->checkAtOnce(selectedRows);
    });
    QAction *editRule=new QAction(tr("Edit"), this);
    QObject::connect(editRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
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
    QAction *removeRule = new QAction(tr("Remove Rule(s)"), this);
    QObject::connect(removeRule, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        GlobalObjects::autoDownloadManager->removeRules(selectedRows);
    });
    QMenu *ruleContextMenu = new ElaMenu(ruleView);
    ruleContextMenu->addAction(enbaleRule);
    ruleContextMenu->addAction(disableRule);
    ruleContextMenu->addAction(checkRule);
    ruleContextMenu->addAction(editRule);
    ruleContextMenu->addAction(removeRule);
    QObject::connect(ruleView, &QTreeView::customContextMenuRequested, ruleView, [=](){
        QModelIndexList selectedRows = ruleView->selectionModel()->selectedRows();
        if(selectedRows.empty()) return;
        ruleContextMenu->exec(QCursor::pos());
    });

    QObject::connect(ruleView, &QTreeView::doubleClicked, this, [](const QModelIndex &index){
        DownloadRule *rule = GlobalObjects::autoDownloadManager->getRule(index);
        if (rule->state == 0)
        {
            GlobalObjects::autoDownloadManager->stopRules({index});
        }
        else if (rule->state == 2)
        {
            GlobalObjects::autoDownloadManager->startRules({index});
        }
    });
    QObject::connect(ruleView->selectionModel(), &QItemSelectionModel::selectionChanged,this,[this](){
        QModelIndexList selectedRows= ruleView->selectionModel()->selectedRows();
        if (selectedRows.size()==0)
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

    QAction *copyLog = new QAction(tr("Copy"), this);
    QObject::connect(copyLog, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows(logView->selectionModel()->selectedRows());
        if (selectedRows.empty()) return;
        QModelIndex index = static_cast<LogFilterProxyModel *>(logView->model())->mapToSource(selectedRows.last());
        QClipboard *cb = QApplication::clipboard();
        cb->setText(GlobalObjects::autoDownloadManager->logModel->getLog(index));
    });
    QMenu *logContextMenu = new ElaMenu(logView);
    logContextMenu->addAction(copyLog);
    QObject::connect(logView, &QTreeView::customContextMenuRequested, logView, [=](){
        QModelIndexList selectedRows(logView->selectionModel()->selectedRows());
        if (selectedRows.empty()) return;
        logContextMenu->exec(QCursor::pos());
    });

    QAction *addToDownload = new QAction(tr("Add URI Task"), this);
    QObject::connect(addToDownload, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= urlView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        QStringList uris(GlobalObjects::autoDownloadManager->urlModel->getSelectedURIs(selectedRows));
        emit this->addTask(uris, false, GlobalObjects::autoDownloadManager->urlModel->getRule()->filePath);
    });
    QAction *copyURIs = new QAction(tr("Copy URI"), this);
    QObject::connect(copyURIs, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= urlView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        QStringList uris(GlobalObjects::autoDownloadManager->urlModel->getSelectedURIs(selectedRows));
        QClipboard *cb = QApplication::clipboard();
        cb->setText(uris.join('\n'));
    });
    QAction *removeURIs = new QAction(tr("Remove"), this);
    QObject::connect(removeURIs, &QAction::triggered, this, [this](){
        QModelIndexList selectedRows= urlView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        GlobalObjects::autoDownloadManager->urlModel->removeSelectedURIs(selectedRows);
    });
    QMenu *urlContextMenu = new ElaMenu(urlView);
    urlContextMenu->addAction(addToDownload);
    urlContextMenu->addAction(copyURIs);
    urlContextMenu->addAction(removeURIs);
    QObject::connect(urlView, &QTreeView::customContextMenuRequested, urlView, [=](){
        QModelIndexList selectedRows= urlView->selectionModel()->selectedRows();
        if (selectedRows.empty()) return;
        urlContextMenu->exec(QCursor::pos());
    });
}
