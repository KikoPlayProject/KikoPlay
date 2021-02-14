#include "selecttorrentfile.h"

#include <QTreeView>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include "Common/notifier.h"
#include "widgets/dirselectwidget.h"
#include "Download/torrent.h"
SelectTorrentFile::SelectTorrentFile(TorrentFile *torrentFileTree, QWidget *parent) : CFramelessDialog(tr("Add Torrent"),parent,true),model(nullptr)
{
    model=new TorrentFileModel(torrentFileTree,this);
    QTreeView *torrentFileView=new QTreeView(this);
    torrentFileView->setAlternatingRowColors(true);
    torrentFileView->setModel(model);
    torrentFileView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    torrentFileView->header()->resizeSection(0,240*logicalDpiX()/96);
    torrentFileView->header()->resizeSection(1,50*logicalDpiX()/96);
    torrentFileView->header()->resizeSection(2,50*logicalDpiX()/96);


    QCheckBox *selectAll=new QCheckBox(tr("Select All"),this);
    selectAll->setChecked(true);
    QObject::connect(selectAll,&QCheckBox::stateChanged,[this](int state){
        model->checkAll(state==Qt::Checked);
    });
    QCheckBox *selectVideo=new QCheckBox(tr("Select Video"),this);
    QObject::connect(selectVideo,&QCheckBox::stateChanged,[this](int state){
        model->checkVideoFiles(state==Qt::Checked);
    });
    QLabel *checkedFileSizeLabel=new QLabel(this);
    QObject::connect(model,&TorrentFileModel::checkedIndexChanged,[this,checkedFileSizeLabel](){
        checkedFileSize=model->getCheckedFileSize();
        checkedFileSizeLabel->setText(tr("Select: %1").arg(formatSize(false,checkedFileSize)));
    });

    checkedFileSize=model->getCheckedFileSize();
    checkedFileSizeLabel->setText(tr("Select: %1").arg(formatSize(false,checkedFileSize)));
    dirSelect=new DirSelectWidget(this);

    QVBoxLayout *dialogVLayout=new QVBoxLayout(this);
    QHBoxLayout *checkHLayout=new QHBoxLayout();
    checkHLayout->addWidget(selectAll);
    checkHLayout->addWidget(selectVideo);
    checkHLayout->addStretch(1);
    checkHLayout->addWidget(checkedFileSizeLabel);
    dialogVLayout->addLayout(checkHLayout);
    dialogVLayout->addWidget(torrentFileView);
    dialogVLayout->addWidget(dirSelect);

    resize(400*logicalDpiX()/96,420*logicalDpiY()/96);
}

void SelectTorrentFile::onAccept()
{
    QString selectIndexes=model->getCheckedIndex();
    if(selectIndexes.isEmpty())
    {
        showMessage(tr("No File is Selected"), NM_ERROR | NM_HIDE);
        return;
    }
    this->selectIndexes=selectIndexes;
    if(!dirSelect->isValid())
    {
        showMessage(tr("Dir is invaild"), NM_ERROR | NM_HIDE);
        return;
    }
    if(checkedFileSize>dirSelect->getFreeSpace())
    {
        showMessage(tr("Insufficient Disk Space"), NM_ERROR | NM_HIDE);
        return;
    }
    this->dir=dirSelect->getDir();
    CFramelessDialog::onAccept();
}
