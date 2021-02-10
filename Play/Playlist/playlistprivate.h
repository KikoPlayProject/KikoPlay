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
    int modifyCounter;
    bool saveFinishTimeOnce;

    QList<PlayListItem *> itemsClipboard;
    QList<QPair<QString,QString> > recentList;
    QHash<QString,PlayListItem *> fileItems, bgmCollectionItems;

public:
    void loadPlaylist();
    void savePlaylist();
    void incModifyCounter();

    void loadRecentlist();
    void saveRecentlist();
    void updateRecentlist(PlayListItem *item);

    PlayListItem *getPrevOrNextItem(bool prev);
    bool addSubFolder(QString folderStr, PlayListItem *parent, int &itemCount);
    int refreshFolder(PlayListItem *folderItem, QList<PlayListItem *> &nItems);

    QString setCollectionTitle(QList<PlayListItem *> &list);
    void dumpItem(QJsonArray &array,PlayListItem *item, QHash<QString, QString> &mediaHash);
private:
    void saveItem(QXmlStreamWriter &writer,PlayListItem *item);
private:
    PlayList *const q_ptr;
    Q_DECLARE_PUBLIC(PlayList)
    QString plPath;
    QString rectPath;
};

#endif // PLAYLISTPRIVATE_H
