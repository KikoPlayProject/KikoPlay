#include "ressearchwindow.h"
#include "Download/Script/scriptmanager.h"
#include "managescript.h"
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QTreeView>
#include <QHeaderView>
#include <QLabel>
#include <QAction>
#include <QSortFilterProxyModel>
#include <QGridLayout>
#include <QMessageBox>
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QUrl>
#include <QIntValidator>
#include <QMovie>
#include "widgets/dialogtip.h"
#include "globalobjects.h"
namespace
{
    QMap<int,QList<ResItem> > pageCache;
}
ResSearchWindow::ResSearchWindow(QWidget *parent) : QWidget(parent),totalPage(0),currentPage(0),isSearching(false)
{
    searchListModel=new SearchListModel(this);
    QSortFilterProxyModel *searchProxyModel=new QSortFilterProxyModel(this);
    searchProxyModel->setSourceModel(searchListModel);
    searchProxyModel->setFilterKeyColumn(1);
    searchProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    scriptCombo=new QComboBox(this);
    scriptCombo->setProperty("cScrollStyle", true);
    QObject::connect(GlobalObjects::scriptManager,&ScriptManager::refreshDone,this,[this](){
        scriptCombo->clear();
        for(auto &item:GlobalObjects::scriptManager->getScriptList())
        {
            scriptCombo->addItem(item.title,item.id);
        }
        scriptCombo->setCurrentIndex(scriptCombo->findData(GlobalObjects::scriptManager->getNormalScriptId()));
    });

    QPushButton *manageScript=new QPushButton(tr("Manage Script"),this);
    QObject::connect(manageScript,&QPushButton::clicked,this,[this](){
        ManageScript manager(this);
        manager.exec();
    });

    searchEdit=new QLineEdit(this);
    searchEdit->setClearButtonEnabled(true);
    searchEdit->setPlaceholderText(tr("Search"));
    QObject::connect(searchEdit,&QLineEdit::returnPressed,[this](){
        search(searchEdit->text().trimmed());
    });

    QMovie *downloadingIcon=new QMovie(this);
    busyLabel=new QLabel(this);
    busyLabel->setMovie(downloadingIcon);
    downloadingIcon->setFileName(":/res/images/loading-blocks.gif");
    busyLabel->setFixedSize(24*logicalDpiX()/96,24*logicalDpiY()/96);
    busyLabel->setScaledContents(true);
    downloadingIcon->start();
    busyLabel->hide();

    QLineEdit *filterEdit=new QLineEdit(this);
    filterEdit->setPlaceholderText(tr("Filter"));
    filterEdit->setClearButtonEnabled(true);
    QObject::connect(filterEdit,&QLineEdit::textChanged,[searchProxyModel](const QString &text){
        searchProxyModel->setFilterRegExp(text);
    });

    prevPage=new QPushButton(this);
    GlobalObjects::iconfont.setPointSize(11);
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

    dialogTip = new DialogTip(this);
    dialogTip->hide();

    searchListView=new QTreeView(this);
    searchListView->setObjectName(QStringLiteral("SearchListView"));
    searchListView->setProperty("cScrollStyle", true);
    searchListView->setModel(searchProxyModel);
    searchListView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);
    searchListView->setRootIsDecorated(false);
    searchListView->setAlternatingRowColors(true);
    searchListView->setFont(QFont("Microsoft Yahei UI",10));
    searchListView->setIndentation(0);
    searchListView->setContextMenuPolicy(Qt::ActionsContextMenu);
    QObject::connect(searchListView, &QTreeView::doubleClicked,[this,searchProxyModel](const QModelIndex &index){
        ResItem item = searchListModel->getItem(searchProxyModel->mapToSource(index));
        if(item.magnet.isEmpty())
        {
            if(!item.url.isEmpty())
            {
                QDesktopServices::openUrl(QUrl(item.url));
            }
            return;
        }
        emit addTask(QStringList()<<item.magnet);
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
        ResItem item = searchListModel->getItem(selection.indexes().last());
        if(item.url.isEmpty()) return;
        QDesktopServices::openUrl(QUrl(item.url));
    });
    searchListView->addAction(download);
    searchListView->addAction(copyTitle);
    searchListView->addAction(copyMagnet);
    searchListView->addAction(openLink);

    QHBoxLayout *btnHLayout=new QHBoxLayout();
    btnHLayout->addWidget(manageScript);
    btnHLayout->addWidget(scriptCombo);
    btnHLayout->addWidget(searchEdit);
    btnHLayout->addWidget(busyLabel);
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
    if(scriptCombo->currentText().isEmpty()) return;
    if(setSearchEdit) searchEdit->setText(keyword);
    isSearching=true;
    int pageCount;
    QList<ResItem> results;
    setEnable(false);
    QString errInfo(GlobalObjects::scriptManager->search(scriptCombo->currentData().toString(),keyword,1,pageCount,results));
    if(errInfo.isEmpty())
    {
        currentPage= pageCount>0?1:0;
        totalPage=pageCount;
        currentScriptId=scriptCombo->currentData().toString();
        currentKeyword=keyword;
        pageCache.clear();
        searchListModel->setList(results);
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/%1").arg(totalPage));
    }
    else
    {
        //QMessageBox::information(this,"KikoPlay",errInfo);
        dialogTip->showMessage(errInfo, 1);
        dialogTip->raise();
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
    busyLabel->setVisible(!on);
}

void ResSearchWindow::pageTurning(int page)
{
    if(currentKeyword.isEmpty() || currentScriptId.isEmpty()) return;
    if(pageCache.contains(page))
    {
        auto list=pageCache.value(page);
        searchListModel->setList(list);
        if(!pageCache.contains(currentPage)) pageCache.insert(currentPage,list);
        currentPage=page;
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/%1").arg(totalPage));
        return;
    }
    int pageCount;
    QList<ResItem> results;
    setEnable(false);
    QString errInfo(GlobalObjects::scriptManager->search(currentScriptId,currentKeyword,page,pageCount,results));
    if(errInfo.isEmpty())
    {
        searchListModel->setList(results);
        if(!pageCache.contains(currentPage)) pageCache.insert(currentPage,results);
        currentPage=page;
        totalPage=pageCount;
        pageEdit->setText(QString::number(currentPage));
        totalPageTip->setText(QString("/%1").arg(totalPage));
    }
    else
    {
        dialogTip->showMessage(errInfo, 1);
        dialogTip->raise();
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
