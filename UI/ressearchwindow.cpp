#include "ressearchwindow.h"
#include "Extension/Script/scriptmanager.h"
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTreeView>
#include <QHeaderView>
#include <QLabel>
#include <QAction>
#include <QSortFilterProxyModel>
#include <QGridLayout>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QIntValidator>
#include <QMovie>
#include "Common/notifier.h"
#include "UI/ela/ElaMenu.h"
#include "UI/widgets/floatscrollbar.h"
#include "UI/widgets/fonticonbutton.h"
#include "UI/widgets/klineedit.h"
#include "globalobjects.h"
#include <QToolButton>
#include <QWidgetAction>
#include <QMouseEvent>
#include <QGraphicsOpacityEffect>
#include "widgets/scriptsearchoptionpanel.h"
namespace
{
    QMap<int,QList<ResourceItem> > pageCache;
}
ResSearchWindow::ResSearchWindow(QWidget *parent) : QWidget(parent),totalPage(0),currentPage(0),isSearching(false)
{
    searchListModel = new SearchListModel(this);
    searchProxyModel = new QSortFilterProxyModel(this);
    searchProxyModel->setSourceModel(searchListModel);
    searchProxyModel->setFilterKeyColumn((int)SearchListModel::Columns::TITLE);
    searchProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(searchListModel, &SearchListModel::fetching, this, [=](bool on){
        setEnable(!on);
    });

    scriptOptionPanel = new ScriptSearchOptionPanel(this);

    searchEdit = new KLineEdit(this);
    searchEdit->setObjectName(QStringLiteral("FilterEdit"));
    searchEdit->setFont(QFont(GlobalObjects::normalFont, 14));
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setPlaceholderText(tr("Search"));

    QMenu *scriptMenu = new ElaMenu(this);
    scriptCheckGroup = new QActionGroup(this);
    auto updateScriptOptionPanel = [=](const QString &id){
        scriptOptionPanel->setScript(GlobalObjects::scriptManager->getScript(id));
        if (scriptOptionPanel->hasOptions()) scriptOptionPanel->show();
        else scriptOptionPanel->hide();
        if (!scriptOptionPanel->isHidden())
        {
            QGridLayout * gLayout = static_cast<QGridLayout *>(this->layout());
            if(!gLayout || !searchListView) return;
            int optionAvailableWidth = searchListView->width() - 20 -
                                       gLayout->cellRect(0, 0).width() - gLayout->cellRect(0, 2).width();
            if(scriptOptionPanel->getAllWidth() < optionAvailableWidth)
            {
                gLayout->addWidget(scriptOptionPanel, 0, 1);
            }
            else
            {
                gLayout->addWidget(scriptOptionPanel, 1, 0, 1, 3);
            }
        }
    };
    auto refreshScripts = [=](){
        scriptMenu->clear();
        for (auto &s :GlobalObjects::scriptManager->scripts(ScriptType::RESOURCE))
        {
            QAction *scriptAct = scriptMenu->addAction(s->name());
            scriptCheckGroup->addAction(scriptAct);
            scriptAct->setCheckable(true);
            scriptAct->setData(s->id());
        }
        if (!scriptCheckGroup->actions().empty())
        {
            QAction *act = scriptCheckGroup->actions().first();
            act->setChecked(true);
            searchEdit->setPlaceholderText(tr("Search: %1").arg(act->text()));
            updateScriptOptionPanel(act->data().toString());
        }
    };
    QObject::connect(scriptCheckGroup, &QActionGroup::triggered, this, [=](QAction *act){
        searchEdit->setPlaceholderText(tr("Search: %1").arg(act->text()));
        updateScriptOptionPanel(act->data().toString());
    });
    refreshScripts();
    QObject::connect(GlobalObjects::scriptManager,&ScriptManager::scriptChanged, this, [=](ScriptType type){
        if (type != ScriptType::RESOURCE) return;
        refreshScripts();
    });

    QToolButton *optionsButton = new QToolButton;
    optionsButton->setCursor(Qt::ArrowCursor);
    optionsButton->setFocusPolicy(Qt::NoFocus);
    optionsButton->setObjectName(QStringLiteral("FilterOptionButton"));
    GlobalObjects::iconfont->setPointSize(14);
    optionsButton->setFont(*GlobalObjects::iconfont);
    optionsButton->setText(QChar(0xea8a));
    optionsButton->setMenu(scriptMenu);
    optionsButton->setPopupMode(QToolButton::InstantPopup);
    QWidgetAction *optionsAction = new QWidgetAction(this);
    optionsAction->setDefaultWidget(optionsButton);
    searchEdit->addAction(optionsAction, QLineEdit::LeadingPosition);

    QObject::connect(searchEdit,&QLineEdit::returnPressed,[this](){
        search(searchEdit->text().trimmed());
    });

    KLineEdit *filterEdit = new KLineEdit(this);
    filterEdit->setPlaceholderText(tr("Filter"));
    filterEdit->setObjectName(QStringLiteral("FilterEdit"));
    filterEdit->setFont(QFont(GlobalObjects::normalFont, 14));
    filterEdit->setClearButtonEnabled(true);
    QMargins textMargins = filterEdit->textMargins();
    textMargins.setLeft(6);
    filterEdit->setTextMargins(textMargins);

    QObject::connect(filterEdit,&QLineEdit::textChanged, searchProxyModel, [=](const QString &text){
        searchProxyModel->setFilterRegExp(text);
    });

    pageTurningContainer = new QWidget(this);
    QGraphicsOpacityEffect *pageOpacity = new QGraphicsOpacityEffect(pageTurningContainer);
    pageOpacity->setOpacity(0.6);
    pageTurningContainer->setGraphicsEffect(pageOpacity);
    pageTurningContainer->setObjectName(QStringLiteral("PageTurningContainer"));
    pageTurningContainer->hide();
    pageTurningContainer->installEventFilter(this);
    prevPage = new FontIconButton(QChar(0xe617), "", 14, 10, 2, this);
    prevPage->setObjectName(QStringLiteral("PageTurningToolButton"));
    prevPage->setContentsMargins(2, 2, 2, 2);
    QObject::connect(prevPage, &QPushButton::clicked, this, [this](){
        if (currentPage <= 1) return;
        pageTurning(currentPage - 1);
    });

    nextPage = new FontIconButton(QChar(0xe623), "", 14, 10, 2, this);
    nextPage->setObjectName(QStringLiteral("PageTurningToolButton"));
    nextPage->setContentsMargins(2, 2, 2, 2);
    QObject::connect(nextPage, &QPushButton::clicked, this, [this](){
        if (currentPage == totalPage) return;
        pageTurning(currentPage + 1);
    });

    pageEdit = new KLineEdit("0",this);
    pageEdit->setObjectName(QStringLiteral("PageEdit"));
    pageEdit->setFixedWidth(60);
    pageEdit->setAlignment(Qt::AlignCenter);
    pageEdit->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
    pageEdit->setValidator(new QIntValidator(pageEdit));
    QObject::connect(pageEdit,&QLineEdit::returnPressed,[this](){
        int page= qBound(totalPage==0?0:1,pageEdit->text().toInt(),totalPage==0?0:totalPage);
        if(page==currentPage) return;
        pageTurning(page);
    });

    totalPageTip = new QLabel("/ 0 ", this);
    totalPageTip->setObjectName(QStringLiteral("ResTotalPage"));
    QWidgetAction *pageTipAction = new QWidgetAction(this);
    pageTipAction->setDefaultWidget(totalPageTip);
    pageEdit->addAction(pageTipAction, QLineEdit::TrailingPosition);


    searchListView = new QTreeView(this);
    // searchListView->setObjectName(QStringLiteral("TaskInfoTreeView"));
    // searchListView->header()->setObjectName(QStringLiteral("TaskInfoTreeViewHeader"));
    new FloatScrollBar(searchListView->verticalScrollBar(), searchListView);
    new FloatScrollBar(searchListView->horizontalScrollBar(), searchListView);
    searchListView->setModel(searchProxyModel);
    searchListView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    searchListView->setRootIsDecorated(false);
    searchListView->setAlternatingRowColors(true);
    searchListView->setFont(QFont(GlobalObjects::normalFont,10));
    searchListView->setIndentation(0);
    searchListView->setContextMenuPolicy(Qt::CustomContextMenu);
    searchListView->setSortingEnabled(true);


    QObject::connect(searchListView, &QTreeView::doubleClicked, this, [=](const QModelIndex &index){
        QModelIndex selIndex = searchProxyModel->mapToSource(index);
        emit addTask(searchListModel->getMagnetList({selIndex.siblingAtColumn((int)SearchListModel::Columns::TIME)}));
    });
    QAction *download = new QAction(tr("Download"),this);
    QObject::connect(download,&QAction::triggered,this,[this](){
        QItemSelection selection = searchProxyModel->mapSelectionToSource(searchListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        emit addTask(searchListModel->getMagnetList(selection.indexes()));
    });
    QAction *copyTitle = new QAction(tr("Copy Title"),this);
    QObject::connect(copyTitle,&QAction::triggered,this,[this](){
        QItemSelection selection = searchProxyModel->mapSelectionToSource(searchListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(searchListModel->getItem(selection.indexes().last()).title);
    });
    QAction *copyMagnet = new QAction(tr("Copy Magnet"),this);
    QObject::connect(copyMagnet,&QAction::triggered,this,[this](){
        QItemSelection selection = searchProxyModel->mapSelectionToSource(searchListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(searchListModel->getMagnetList(selection.indexes()).join('\n'));
    });
    QAction *openLink=new QAction(tr("Open Link"),this);
    QObject::connect(openLink,&QAction::triggered,this,[this](){
        QItemSelection selection = searchProxyModel->mapSelectionToSource(searchListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        ResourceItem item = searchListModel->getItem(selection.indexes().last());
        if(item.url.isEmpty()) return;
        QDesktopServices::openUrl(QUrl(item.url));
    });

    QMenu *searchListContextMenu = new ElaMenu(searchListView);
    searchListContextMenu->addAction(download);
    searchListContextMenu->addAction(copyTitle);
    searchListContextMenu->addAction(copyMagnet);
    searchListContextMenu->addAction(openLink);
    QObject::connect(searchListView, &QTreeView::customContextMenuRequested, searchListView, [=](){
        bool hasSelection = !searchListView->selectionModel()->selection().empty();
        download->setEnabled(hasSelection);
        copyTitle->setEnabled(hasSelection);
        copyMagnet->setEnabled(hasSelection);
        openLink->setEnabled(hasSelection);
        searchListContextMenu->exec(QCursor::pos());
    });

    QHBoxLayout *pageHLayout = new QHBoxLayout(pageTurningContainer);
    pageHLayout->setSpacing(1);
    pageHLayout->setContentsMargins(4, 4, 4, 6);
    pageHLayout->addWidget(prevPage);
    pageHLayout->addSpacing(4);
    pageHLayout->addWidget(pageEdit);
    pageHLayout->addSpacing(4);
    pageHLayout->addWidget(nextPage);

    QGridLayout *searchWindowGLayout = new QGridLayout(this);
    searchWindowGLayout->addWidget(searchEdit, 0, 0);
    searchWindowGLayout->addWidget(filterEdit, 0, 2);
    searchWindowGLayout->addWidget(searchListView, 2, 0, 1, 3);
    searchWindowGLayout->setRowStretch(2,1);
    searchWindowGLayout->setColumnStretch(1,1);
}

void ResSearchWindow::search(const QString &keyword, bool setSearchEdit)
{
    if (isSearching) return;
    if (keyword.isEmpty()) return;
    if (!scriptCheckGroup->checkedAction() || scriptCheckGroup->checkedAction()->data().isNull()) return;
    auto curScript = GlobalObjects::scriptManager->getScript(scriptCheckGroup->checkedAction()->data().toString());
    if(!curScript || curScript->type()!=ScriptType::RESOURCE) return;
    ResourceScript *resScript = static_cast<ResourceScript *>(curScript.data());
    if(setSearchEdit) searchEdit->setText(keyword);
    isSearching=true;
    int pageCount;
    QList<ResourceItem> results;
    setEnable(false);
    Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Searching..."), NM_PROCESS | NM_DARKNESS_BACK);
    if (scriptOptionPanel->hasOptions() && scriptOptionPanel->changed())
    {
        QMap<QString, QString> searchOptions = scriptOptionPanel->getOptionVals();
        for(auto iter = searchOptions.cbegin(); iter != searchOptions.cend(); ++iter)
        {
            resScript->setSearchOption(iter.key(), iter.value());
        }
    }
    ScriptState state = resScript->search(keyword, 1, pageCount, results);
    if (state)
    {
        currentPage = pageCount>0?1:0;
        totalPage = pageCount;
        currentScriptId = scriptCheckGroup->checkedAction()->data().toString();
        currentKeyword = keyword;
        pageCache.clear();
        searchListModel->setList(currentScriptId, results);
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/ %1 ").arg(totalPage));
        totalPageTip->adjustSize();
        searchProxyModel->sort(-1);
        Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Down"), NM_HIDE);
    }
    else
    {
        Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, state.info, NM_ERROR | NM_HIDE);
    }
    pageTurningContainer->resize(pageTurningContainer->layout()->sizeHint());
    pageTurningContainer->raise();
    pageTurningContainer->move((width()-pageTurningContainer->width())/2, (height() - pageTurningContainer->height()-40));
    pageTurningContainer->setVisible(totalPage > 1);
    isSearching = false;
    setEnable(true);
}

void ResSearchWindow::setEnable(bool on)
{
    searchEdit->setEnabled(on);
    prevPage->setEnabled(on);
    nextPage->setEnabled(on);
    pageEdit->setEnabled(on);
}

void ResSearchWindow::pageTurning(int page)
{
    if (currentKeyword.isEmpty() || currentScriptId.isEmpty()) return;
    if (pageCache.contains(page))
    {
        auto list = pageCache.value(page);
        searchListModel->setList(currentScriptId, list);
        if (!pageCache.contains(currentPage)) pageCache.insert(currentPage,list);
        currentPage = page;
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/ %1 ").arg(totalPage));
        totalPageTip->adjustSize();
        searchListView->scrollToTop();
        return;
    }
    auto curScript = GlobalObjects::scriptManager->getScript(currentScriptId);
    if (!curScript || curScript->type()!=ScriptType::RESOURCE) return;
    ResourceScript *resScript = static_cast<ResourceScript *>(curScript.data());
    int pageCount = 0;
    QList<ResourceItem> results;
    setEnable(false);
    Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Searching..."), NM_PROCESS | NM_DARKNESS_BACK);
    ScriptState state = resScript->search(currentKeyword, page, pageCount, results);
    if (state)
    {
        searchListModel->setList(currentScriptId, results);
        if(!pageCache.contains(currentPage)) pageCache.insert(currentPage, results);
        currentPage = page;
        totalPage = pageCount;
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/ %1 ").arg(totalPage));
        totalPageTip->adjustSize();
        searchListView->scrollToTop();
        Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Down"), NM_HIDE);
    }
    else
    {
        Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, state.info, NM_ERROR | NM_HIDE);
    }
    pageTurningContainer->resize(pageTurningContainer->layout()->sizeHint());
    pageTurningContainer->raise();
    pageTurningContainer->move((width()-pageTurningContainer->width())/2, (height() - pageTurningContainer->height()-40));
    pageTurningContainer->setVisible(totalPage > 1);
    setEnable(true);
}

void ResSearchWindow::resizeEvent(QResizeEvent *)
{
    int oneWidth = searchListView->width() / 6;
    searchListView->header()->resizeSection(0, oneWidth);
    searchListView->header()->resizeSection(1, 4*oneWidth);
    searchListView->header()->resizeSection(2, 1*oneWidth);
    if (!scriptOptionPanel->isHidden())
    {
        QGridLayout * gLayout = static_cast<QGridLayout *>(this->layout());
        if (!gLayout || !searchListView) return;
        int optionAvailableWidth = searchListView->width() - 20 -
                gLayout->cellRect(0, 0).width() - gLayout->cellRect(0, 2).width();
        if (scriptOptionPanel->getAllWidth() < optionAvailableWidth)
        {
            gLayout->addWidget(scriptOptionPanel, 0, 1);
        }
        else
        {
            gLayout->addWidget(scriptOptionPanel, 1, 0, 1, 3);
        }
    }
    pageTurningContainer->resize(pageTurningContainer->layout()->sizeHint());
    pageTurningContainer->raise();
    pageTurningContainer->move((width()-pageTurningContainer->width())/2, (height() - pageTurningContainer->height()-40));
}

bool ResSearchWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == pageTurningContainer)
    {
        if (event->type() == QEvent::Enter)
        {
            static_cast<QGraphicsOpacityEffect *>(pageTurningContainer->graphicsEffect())->setEnabled(false);
        }
        else if (event->type() == QEvent::Leave)
        {
            static_cast<QGraphicsOpacityEffect *>(pageTurningContainer->graphicsEffect())->setEnabled(true);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void SearchListModel::setList(const QString &curScriptId, QList<ResourceItem> &nList)
{
    beginResetModel();
    scriptId = curScriptId;
    resultList.swap(nList);
    endResetModel();
}
QStringList SearchListModel::getMagnetList(const QModelIndexList &indexes)
{
    QStringList list;
    auto curScript = GlobalObjects::scriptManager->getScript(scriptId);
    if (curScript)
    {
        ResourceScript *resScript = static_cast<ResourceScript *>(curScript.data());
        if (resScript->needGetDetail())
        {
            Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Fetching Magnet..."), NM_PROCESS | NM_DARKNESS_BACK);
            emit fetching(true);
            for (auto &index : indexes)
            {
                if (index.column() == (int)Columns::TIME)
                {
                    int row = index.row();
                    ScriptState state;
                    if (resultList[row].magnet.isEmpty())
                    {
                        state = resScript->getDetail(resultList[row], resultList[row]);
                    }
                    if (state) list<<resultList[row].magnet;
                }
            }
            Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Down"), NM_HIDE);
            emit fetching(false);
            return list;
        }
    }
    for (auto &index : indexes)
    {
        if (index.column() == (int)Columns::TIME)
        {
            list << resultList.at(index.row()).magnet;
        }
    }
    return list;
}

QVariant SearchListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    Columns col=static_cast<Columns>(index.column());
    const ResourceItem &item=resultList.at(index.row());
    if (role == Qt::DisplayRole)
    {
        switch (col)
        {
        case Columns::TIME:
            return item.time;
        case Columns::TITLE:
            return item.title;
        case Columns::SIZE:
            return item.size;
        }
    }
    else if(role == Qt::ToolTipRole)
    {
        if(col == Columns::TITLE)
            return item.title;
    }
    return QVariant();
}

QVariant SearchListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if (section < headers.size()) return headers.at(section);
    }
    return QVariant();
}
