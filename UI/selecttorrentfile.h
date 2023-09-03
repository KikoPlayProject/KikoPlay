#ifndef SELECTTORRENTFILE_H
#define SELECTTORRENTFILE_H

#include "framelessdialog.h"
struct TorrentFile;
class TorrentFileModel;
class DirSelectWidget;
class SelectTorrentFile : public CFramelessDialog
{
    Q_OBJECT
public:
    explicit SelectTorrentFile(TorrentFile *torrentFileTree, QWidget *parent = nullptr, const QString &path = "");
    QString selectIndexes;
    QString dir;
private:
    TorrentFileModel *model;
    DirSelectWidget *dirSelect;
    qint64 checkedFileSize;
    // CFramelessDialog interface
protected:
    virtual void onAccept();
};

#endif // SELECTTORRENTFILE_H
