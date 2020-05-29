#include "util.h"

QString formatSize(bool isSpeed, float val)
{
    static const QStringList sizeUnits={"B","KB","MB","GB","TB"};
    static const QStringList speedUnits={"B/s","KB/s","MB/s","GB/s","TB/s"};
    int i=0;
    for(;i<5;++i)
    {
        if(val<1024.0)
        {
            return QString().setNum(val,'f',2)+ (isSpeed?speedUnits[i]:sizeUnits[i]);
            break;
        }
        val/=1024.0;
    }
    return QString().setNum(val*1024,'f',2)+ (isSpeed?speedUnits[i-1]:sizeUnits[i-1]);
}

TorrentFile::TorrentFile():size(0),completedSize(0),index(-1)
{

}
TorrentFile::~TorrentFile()
{
    qDeleteAll(children);
}
void TorrentFile::setChildrenCheckStatus()
{
    if(checkStatus!=Qt::PartiallyChecked)
    {
        for(TorrentFile *child:children)
        {
            child->checkStatus=checkStatus;
            child->setChildrenCheckStatus();
        }
    }
}

void TorrentFile::setParentCheckStatus()
{
    if(parent)
    {
        int checkStatusStatis[3]={0,0,0};
        for(TorrentFile *child:parent->children)
        {
            checkStatusStatis[child->checkStatus]++;
        }
        if(checkStatusStatis[2]==parent->children.size())
            parent->checkStatus=Qt::Checked;
        else if(checkStatusStatis[0]==parent->children.size())
            parent->checkStatus=Qt::Unchecked;
        else
            parent->checkStatus=Qt::PartiallyChecked;
        parent->setParentCheckStatus();
    }
}

DownloadTask::DownloadTask(): status(Downloading),totalLength(0),completedLength(0),
    downloadSpeed(0),uploadSpeed(0),connections(0),numPieces(0), pieceLength(0),torrentContentState(-1),
    directlyDownload(false),fileInfo(nullptr)
{

}

DownloadTask::~DownloadTask()
{
    if(fileInfo)delete fileInfo;
}

void TorrentFileInfo::setIndexMap()
{
    QList<TorrentFile *> items;
    items.push_back(root);
    while(!items.empty())
    {
        TorrentFile *currentItem=items.takeFirst();
        for(TorrentFile *child:currentItem->children)
        {
            if(child->children.count()>0)
                items.push_back(child);
            else if(child->index>0)
                indexMap.insert(child->index,child);
        }
    }
}
