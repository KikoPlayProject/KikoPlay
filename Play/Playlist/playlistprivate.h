#ifndef PLAYLISTPRIVATE_H
#define PLAYLISTPRIVATE_H
#include "playlist.h"
#include <QXmlStreamWriter>

class PlayListPrivate
{
public:
    PlayListPrivate(PlayList *pl);
    ~PlayListPrivate();

    PlayListItem *root;
    PlayListItem *currentItem;
    bool playListChanged, needRefresh;
    PlayList::LoopMode loopMode;
    bool autoMatch;
    bool autoAddExternal;
    int modifyCounter;
    bool saveFinishTimeOnce;

    QList<PlayListItem *> itemsClipboard;
    ///QList<QPair<QString, QString> > recentList;
    QVector<RecentlyPlayedItem> recentList;
    QHash<QString, PlayListItem *> fileItems, bgmCollectionItems;
    QHash<QString, QString> mediaPathHash;
    QReadWriteLock pathHashLock;

public:
    void loadPlaylist();
    void savePlaylist();
    void incModifyCounter();

    void loadRecentlist();
    void saveRecentlist();
    void updateRecentlist(PlayListItem *item);
    void updateRecentItemInfo(const PlayListItem *item, const QImage &cover);

    PlayListItem *getPrevOrNextItem(bool prev);
    bool addSubFolder(QString folderStr, PlayListItem *parent, int &itemCount);
    int refreshFolder(PlayListItem *folderItem, QVector<PlayListItem *> &nItems);

    QString setCollectionTitle(QList<PlayListItem *> &list);
    void dumpItem(QJsonArray &array, PlayListItem *item);

    void addMediaPathHash(const QVector<PlayListItem *> newItems);

    void pushEpFinishEvent(PlayListItem *item);
private:
    void saveItem(QXmlStreamWriter &writer,PlayListItem *item);
private:
    PlayList *const q_ptr;
    Q_DECLARE_PUBLIC(PlayList)
    QString plPath;
    QString rectPath;
};

#endif // PLAYLISTPRIVATE_H
