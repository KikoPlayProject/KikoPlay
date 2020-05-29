#ifndef DOWNLOADITEM_H
#define DOWNLOADITEM_H
#include <QtCore>
struct TorrentFile
{
    TorrentFile();
    ~TorrentFile();
    void setChildrenCheckStatus();
    void setParentCheckStatus();
    QString name;
    qint64 size;
    qint64 completedSize;
    int index;
    int checkStatus;
    TorrentFile *parent;
    QList<TorrentFile *> children;
};
struct TorrentFileInfo
{
    TorrentFile *root;
    QMap<int,TorrentFile *> indexMap;
    void setIndexMap();
    ~TorrentFileInfo()
    {
        delete root;
    }
};
struct DownloadTask
{
    enum Status
    {
        Downloading,
        Seeding,
        Waiting,
        Paused,
        Complete,
        Error
    };
    Status status;
    QString gid;
    QString taskID;
    QString title;
    QString dir;
    qint64 totalLength;
    qint64 completedLength;
    int downloadSpeed;
    int uploadSpeed;
    int connections;
    int numPieces;
    int pieceLength;
    qint64 createTime;
    qint64 finishTime;
    QString bitfield;


    QString uri;
    int torrentContentState; //-1:unknown 0:no torrent 1:has torrent
    bool directlyDownload; //for magnet: whether to open the SelectTorrentFile dialog
    QByteArray torrentContent;
    QString selectedIndexes;
    TorrentFileInfo *fileInfo;

    DownloadTask();
    ~DownloadTask();
};
Q_DECLARE_OPAQUE_POINTER(DownloadTask *)
QString formatSize(bool isSpeed,float val);
#endif // DOWNLOADITEM_H
