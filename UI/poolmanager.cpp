#include "poolmanager.h"
#include <QTreeView>
#include <QHeaderView>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QGridLayout>
#include <QMessageBox>
#include <QSortFilterProxyModel>
#include "Play/Danmu/danmumanager.h"
#include "globalobjects.h"

PoolManager::PoolManager(QWidget *parent) : CFramelessDialog(tr("Danmu Pool Manager"),parent)
{
    setFont(QFont("Microsoft Yahei UI",10));
    QTreeView *poolView=new QTreeView(this);
    poolView->setRootIsDecorated(false);
    poolView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    poolView->setFont(this->font());
    poolView->setAlternatingRowColors(true);
    poolView->header()->setStretchLastSection(false);

    QSortFilterProxyModel *proxyModel=new QSortFilterProxyModel(this);
    proxyModel->setSourceModel(GlobalObjects::danmuManager);
    poolView->setModel(proxyModel);
    poolView->setSortingEnabled(true);
    proxyModel->setFilterKeyColumn(0);

    QPushButton *exportPool=new QPushButton(tr("Export Pool(s)"),this);
    QPushButton *deletePool=new QPushButton(tr("Delete Pool(s)"),this);

    QObject::connect(exportPool,&QPushButton::clicked,[poolView,this,exportPool,deletePool](){
        auto selection = poolView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        QString directory = QFileDialog::getExistingDirectory(this,
                                    tr("Select folder"), "",
                                    QFileDialog::DontResolveSymlinks | QFileDialog::ShowDirsOnly);
        if (directory.isEmpty())return;

        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(poolView->model());
        QModelIndexList tmpList;
        for(const QModelIndex &index:selection)
        {
            tmpList.append(model->mapToSource(index));
        }

        this->showBusyState(true);
        exportPool->setText(tr("Exporting..."));
        exportPool->setEnabled(false);
        deletePool->setEnabled(false);
        GlobalObjects::danmuManager->exportPool(tmpList,directory);
        this->showBusyState(false);
        exportPool->setText(tr("Export Pool(s)"));
        exportPool->setEnabled(true);
        deletePool->setEnabled(true);
    });
    QObject::connect(deletePool,&QPushButton::clicked,[poolView, this, exportPool, deletePool](){
        auto selection = poolView->selectionModel()->selectedRows();
        if (selection.size() == 0)return;
        if(QMessageBox::Cancel == QMessageBox::information(this,tr("Delete"),tr("Are you sure you want to delete these Danmu Pool?"),
                                 QMessageBox::Yes|QMessageBox::Cancel,QMessageBox::Cancel))
            return;

        QSortFilterProxyModel *model = static_cast<QSortFilterProxyModel *>(poolView->model());
        QModelIndexList tmpList;
        for(const QModelIndex &index:selection)
        {
            tmpList.append(model->mapToSource(index));
        }

        this->showBusyState(true);
        deletePool->setText(tr("Deleting..."));
        exportPool->setEnabled(false);
        deletePool->setEnabled(false);
        GlobalObjects::danmuManager->deletePool(tmpList);
        this->showBusyState(false);
        deletePool->setText(tr("Delete Pool(s)"));
        exportPool->setEnabled(true);
        deletePool->setEnabled(true);
    });

    QLineEdit *searchEdit=new QLineEdit(this);
    searchEdit->setPlaceholderText(tr("Search"));
    searchEdit->setMinimumWidth(150*logicalDpiX()/96);
    searchEdit->setClearButtonEnabled(true);
    QObject::connect(searchEdit,&QLineEdit::textChanged,[proxyModel](const QString &keyword){
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterRegExp(keyword);
    });

    QGridLayout *managerGLayout=new QGridLayout(this);
    managerGLayout->addWidget(exportPool,0,0);
    managerGLayout->addWidget(deletePool,0,1);
    managerGLayout->addWidget(searchEdit,0,3);
    managerGLayout->addWidget(poolView,1,0,1,4);
    managerGLayout->setRowStretch(1,1);
    managerGLayout->setColumnStretch(2,1);
    managerGLayout->setContentsMargins(0, 0, 0, 0);
    resize(620*logicalDpiX()/96, 420*logicalDpiY()/96);
    QHeaderView *poolHeader = poolView->header();
    poolHeader->setFont(this->font());
    poolHeader->resizeSection(0, 160*logicalDpiX()/96); //Anime
    poolHeader->resizeSection(1, 260*logicalDpiX()/96); //Ep
    poolHeader->resizeSection(2, 120*logicalDpiX()/96); //Count

    GlobalObjects::danmuManager->refreshPoolInfo();
}
