#include "ressearchwindow.h"
#include "Script/scriptmanager.h"
#include "settings.h"
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
#include "widgets/dialogtip.h"
#include "Common/notifier.h"
#include "globalobjects.h"
#include "stylemanager.h"
namespace
{
    QMap<int,QList<ResourceItem> > pageCache;
}
ResSearchWindow::ResSearchWindow(QWidget *parent) : QWidget(parent),totalPage(0),currentPage(0),isSearching(false)
{
    searchListModel=new SearchListModel(this);
    QSortFilterProxyModel *searchProxyModel=new QSortFilterProxyModel(this);
    searchProxyModel->setSourceModel(searchListModel);
    searchProxyModel->setFilterKeyColumn((int)SearchListModel::Columns::TITLE);
    searchProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    QObject::connect(searchListModel, &SearchListModel::fetching, this, [=](bool on){
        setEnable(!on);
    });

    scriptCombo=new QComboBox(this);
    scriptCombo->setProperty("cScrollStyle", true);
    auto scripts = GlobalObjects::scriptManager->scripts(ScriptType::RESOURCE);
    for(auto &s : scripts)
    {
        scriptCombo->addItem(s->name(), s->id());
    }
    QObject::connect(GlobalObjects::scriptManager,&ScriptManager::scriptChanged, this,[this](ScriptType type){
        if(type!=ScriptType::RESOURCE) return;
        scriptCombo->clear();
        for(auto &s:GlobalObjects::scriptManager->scripts(ScriptType::RESOURCE))
        {
            scriptCombo->addItem(s->name(), s->id());
        }
        scriptCombo->setCurrentIndex(0);
    });

    searchEdit=new QLineEdit(this);
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setPlaceholderText(tr("Search"));
    QObject::connect(searchEdit,&QLineEdit::returnPressed,[this](){
        search(searchEdit->text().trimmed());
    });

    QLineEdit *filterEdit=new QLineEdit(this);
    filterEdit->setPlaceholderText(tr("Filter"));
    filterEdit->setClearButtonEnabled(true);
    QObject::connect(filterEdit,&QLineEdit::textChanged,[searchProxyModel](const QString &text){
        searchProxyModel->setFilterRegExp(text);
    });

    prevPage=new QPushButton(this);
    GlobalObjects::iconfont.setPointSize(10);
    prevPage->setFont(GlobalObjects::iconfont);
    prevPage->setText(QChar(0xe617));
    QObject::connect(prevPage,&QPushButton::clicked,this,[this](){
        if(currentPage<=1) return;
        pageTurning(currentPage-1);
    });
    nextPage=new QPushButton(this);
    nextPage->setFont(GlobalObjects::iconfont);
    nextPage->setText(QChar(0xe623));
    QObject::connect(nextPage,&QPushButton::clicked,this,[this](){
        if(currentPage==totalPage) return;
        pageTurning(currentPage+1);
    });

    QIntValidator* pageRangeValidator=new QIntValidator(this);
    pageEdit=new QLineEdit("0",this);
    pageEdit->setObjectName(QStringLiteral("PageEdit"));
    pageEdit->setFixedWidth(30*logicalDpiX()/96);
    pageEdit->setAlignment(Qt::AlignCenter);
    pageEdit->setSizePolicy(QSizePolicy::Minimum,QSizePolicy::Minimum);
    pageEdit->setValidator(pageRangeValidator);
    QObject::connect(pageEdit,&QLineEdit::returnPressed,[this](){
        int page= qBound(totalPage==0?0:1,pageEdit->text().toInt(),totalPage==0?0:totalPage);
        if(page==currentPage) return;
        pageTurning(page);
    });

    totalPageTip=new QLabel("/0", this);
    totalPageTip->setObjectName(QStringLiteral("ResTotalPage"));

    searchListView=new QTreeView(this);
    searchListView->setObjectName(QStringLiteral("SearchListView"));
    searchListView->setModel(searchProxyModel);
    searchListView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    searchListView->setRootIsDecorated(false);
    searchListView->setAlternatingRowColors(true);
    searchListView->setFont(QFont(GlobalObjects::normalFont,10));
    searchListView->setIndentation(0);
    searchListView->setContextMenuPolicy(Qt::ActionsContextMenu);

    QObject::connect(StyleManager::getStyleManager(), &StyleManager::styleModelChanged, this, [=](StyleManager::StyleMode mode){
        bool setScrollStyle = (mode==StyleManager::BG_COLOR || mode==StyleManager::DEFAULT_BG);
        searchListView->setProperty("cScrollStyle", setScrollStyle);
    });

    QObject::connect(searchListView, &QTreeView::doubleClicked,[this,searchProxyModel](const QModelIndex &index){
        QModelIndex selIndex = searchProxyModel->mapToSource(index);
        emit addTask(searchListModel->getMagnetList({selIndex.siblingAtColumn((int)SearchListModel::Columns::TIME)}));
    });
    QAction *download=new QAction(tr("Download"),this);
    QObject::connect(download,&QAction::triggered,this,[this,searchProxyModel](){
        QItemSelection selection = searchProxyModel->mapSelectionToSource(searchListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        emit addTask(searchListModel->getMagnetList(selection.indexes()));
    });
    QAction *copyTitle=new QAction(tr("Copy Title"),this);
    QObject::connect(copyTitle,&QAction::triggered,this,[this,searchProxyModel](){
        QItemSelection selection = searchProxyModel->mapSelectionToSource(searchListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(searchListModel->getItem(selection.indexes().last()).title);
    });
    QAction *copyMagnet=new QAction(tr("Copy Magnet"),this);
    QObject::connect(copyMagnet,&QAction::triggered,this,[this,searchProxyModel](){
        QItemSelection selection = searchProxyModel->mapSelectionToSource(searchListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        QClipboard *cb = QApplication::clipboard();
        cb->setText(searchListModel->getMagnetList(selection.indexes()).join('\n'));
    });
    QAction *openLink=new QAction(tr("Open Link"),this);
    QObject::connect(openLink,&QAction::triggered,this,[this,searchProxyModel](){
        QItemSelection selection = searchProxyModel->mapSelectionToSource(searchListView->selectionModel()->selection());
        if (selection.size() == 0)return;
        ResourceItem item = searchListModel->getItem(selection.indexes().last());
        if(item.url.isEmpty()) return;
        QDesktopServices::openUrl(QUrl(item.url));
    });
    searchListView->addAction(download);
    searchListView->addAction(copyTitle);
    searchListView->addAction(copyMagnet);
    searchListView->addAction(openLink);

    QHBoxLayout *btnHLayout=new QHBoxLayout();
    btnHLayout->addWidget(scriptCombo);
    btnHLayout->addWidget(searchEdit);
    btnHLayout->addStretch(1);
    btnHLayout->addWidget(filterEdit);
    QHBoxLayout *pageBarHLayout=new QHBoxLayout();
    pageBarHLayout->setContentsMargins(0,0,0,0);
    pageBarHLayout->setSpacing(1);
    btnHLayout->addWidget(prevPage);
    pageBarHLayout->addWidget(pageEdit);
    pageBarHLayout->addWidget(totalPageTip);

    btnHLayout->addLayout(pageBarHLayout);btnHLayout->addWidget(nextPage);
    QGridLayout *searchWindowGLayout=new QGridLayout(this);
    searchWindowGLayout->addLayout(btnHLayout,0,0);
    searchWindowGLayout->addWidget(searchListView,1,0);

}

void ResSearchWindow::search(const QString &keyword, bool setSearchEdit)
{
    if(isSearching) return;
    if(keyword.isEmpty()) return;
    if(scriptCombo->currentData().isNull()) return;
    auto curScript = GlobalObjects::scriptManager->getScript(scriptCombo->currentData().toString());
    if(!curScript || curScript->type()!=ScriptType::RESOURCE) return;
    ResourceScript *resScript = static_cast<ResourceScript *>(curScript.data());
    if(setSearchEdit) searchEdit->setText(keyword);
    isSearching=true;
    int pageCount;
    QList<ResourceItem> results;
    setEnable(false);
    Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Searching..."), NM_PROCESS | NM_DARKNESS_BACK);
    ScriptState state = resScript->search(keyword, 1, pageCount, results);
    if(state)
    {
        currentPage= pageCount>0?1:0;
        totalPage=pageCount;
        currentScriptId=scriptCombo->currentData().toString();
        currentKeyword=keyword;
        pageCache.clear();
        searchListModel->setList(currentScriptId, results);
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/%1").arg(totalPage));
        Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Down"), NM_HIDE);
    }
    else
    {
        Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, state.info, NM_ERROR | NM_HIDE);
    }
    isSearching=false;
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
    if(currentKeyword.isEmpty() || currentScriptId.isEmpty()) return;
    if(pageCache.contains(page))
    {
        auto list=pageCache.value(page);
        searchListModel->setList(currentScriptId, list);
        if(!pageCache.contains(currentPage)) pageCache.insert(currentPage,list);
        currentPage=page;
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/%1").arg(totalPage));
        return;
    }
    auto curScript = GlobalObjects::scriptManager->getScript(currentScriptId);
    if(!curScript || curScript->type()!=ScriptType::RESOURCE) return;
    ResourceScript *resScript = static_cast<ResourceScript *>(curScript.data());
    int pageCount;
    QList<ResourceItem> results;
    setEnable(false);
    Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Searching..."), NM_PROCESS | NM_DARKNESS_BACK);
    ScriptState state = resScript->search(currentKeyword, page, pageCount, results);
    if(state)
    {
        searchListModel->setList(currentScriptId, results);
        if(!pageCache.contains(currentPage)) pageCache.insert(currentPage,results);
        currentPage=page;
        totalPage=pageCount;
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/%1").arg(totalPage));
        Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Down"), NM_HIDE);
    }
    else
    {
        Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, state.info, NM_ERROR | NM_HIDE);
    }
    setEnable(true);
}

void ResSearchWindow::resizeEvent(QResizeEvent *)
{
    int oneWidth = searchListView->width() / 6;
    searchListView->header()->resizeSection(0,oneWidth);
    searchListView->header()->resizeSection(1,4*oneWidth);
    searchListView->header()->resizeSection(2,1*oneWidth);
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
    if(curScript)
    {
        ResourceScript *resScript = static_cast<ResourceScript *>(curScript.data());
        if(resScript->needGetDetail())
        {
            Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Fetching Magnet..."), NM_PROCESS | NM_DARKNESS_BACK);
            emit fetching(true);
            for(auto &index:indexes)
            {
                if(index.column()==(int)Columns::TIME)
                {
                    int row = index.row();
                    ScriptState state;
                    if(resultList[row].magnet.isEmpty())
                    {
                        state = resScript->getDetail(resultList[row], resultList[row]);
                    }
                    if(state) list<<resultList[row].magnet;
                }
            }
            Notifier::getNotifier()->showMessage(Notifier::DOWNLOAD_NOTIFY, tr("Down"), NM_HIDE);
            emit fetching(false);
            return list;
        }
    }
    for(auto &index:indexes)
    {
        if(index.column()==(int)Columns::TIME)
            list<<resultList.at(index.row()).magnet;
    }
    return list;
}

QVariant SearchListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    Columns col=static_cast<Columns>(index.column());
    const ResourceItem &item=resultList.at(index.row());
    if(role==Qt::DisplayRole)
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
    else if(role==Qt::ToolTipRole)
    {
        if(col==Columns::TITLE)
            return item.title;
    }
    return QVariant();
}

QVariant SearchListModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<headers.size())return headers.at(section);
    }
    return QVariant();
}
