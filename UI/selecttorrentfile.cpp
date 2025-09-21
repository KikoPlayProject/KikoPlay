#include "selecttorrentfile.h"

#include <QTreeView>
#include <QPushButton>
#include <QLabel>
#include <QHBoxLayout>
#include <QHeaderView>
#include "Common/notifier.h"
#include "UI/ela/ElaCheckBox.h"
#include "UI/widgets/component/ktreeviewitemdelegate.h"
#include "widgets/dirselectwidget.h"
#include "Download/torrent.h"
#include "globalobjects.h"

SelectTorrentFile::SelectTorrentFile(TorrentFile *torrentFileTree, QWidget *parent, const QString &path) : CFramelessDialog(tr("Add Torrent"),parent,true),model(nullptr)
{   
    model=new TorrentFileModel(torrentFileTree,this);
    TorrentTreeView *torrentFileView=new TorrentTreeView(this);
    torrentFileView->setObjectName(QStringLiteral("TaskFileInfoView"));
    torrentFileView->setAlternatingRowColors(true);
    torrentFileView->setModel(model);
    torrentFileView->setFont(QFont(GlobalObjects::normalFont, 11));
    torrentFileView->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::MinimumExpanding);
    torrentFileView->setItemDelegate(new KTreeviewItemDelegate(torrentFileView));
    torrentFileView->header()->resizeSection(0,240*logicalDpiX()/96);
    torrentFileView->header()->resizeSection(1,50*logicalDpiX()/96);
    torrentFileView->header()->resizeSection(2,50*logicalDpiX()/96);
    QObject::connect(torrentFileView, &TorrentTreeView::ignoreColorChanged, model, &TorrentFileModel::setIgnoreColor);
    QObject::connect(torrentFileView, &TorrentTreeView::normColorChanged, model, &TorrentFileModel::setNormColor);


    QCheckBox *selectAll = new ElaCheckBox(tr("Select All"),this);
    selectAll->setChecked(true);
    QObject::connect(selectAll, &QCheckBox::stateChanged, this, [this](int state){
        model->checkAll(state==Qt::Checked);
    });
    QCheckBox *selectVideo = new ElaCheckBox(tr("Select Video"), this);
    QObject::connect(selectVideo, &QCheckBox::stateChanged, this, [this](int state){
        model->checkVideoFiles(state==Qt::Checked);
    });
    QLabel *checkedFileSizeLabel=new QLabel(this);
    QObject::connect(model, &TorrentFileModel::checkedIndexChanged, this, [this,checkedFileSizeLabel](){
        checkedFileSize=model->getCheckedFileSize();
        checkedFileSizeLabel->setText(tr("Select: %1").arg(formatSize(false,checkedFileSize)));
    });

    checkedFileSize = model->getCheckedFileSize();
    checkedFileSizeLabel->setText(tr("Select: %1").arg(formatSize(false,checkedFileSize)));
    dirSelect = new DirSelectWidget(this);
    if (!path.isEmpty()) dirSelect->setDir(path);

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
