#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include "playlistitem.h"
#include "MediaLibrary/animeinfo.h"
#include "webdav/qwebdavitem.h"

class PlayListPrivate;
class QWebdav;
class QWebdavDirParser;
class MatchWorker : public QObject
{
    Q_OBJECT
public:
    explicit MatchWorker(QObject *parent = nullptr):QObject(parent) { updateFilterRules(); }
    void match(const QVector<PlayListItem *> &items);
    void match(const QVector<PlayListItem *> &items, const QString &animeTitle, const QList<EpInfo> &eps);
    void updateFilterRules();
signals:
    void matchDown(const QList<PlayListItem *> &matchedItems);
private:
    QVector<QRegularExpression> filterRules;
    bool filterItem(PlayListItem *item);
};

class WebDAVWorker : public QObject
{
    Q_OBJECT
public:
    explicit WebDAVWorker(QObject *parent = nullptr);
    bool listDirectory(PlayListItem *item);

    PlayListItem *currentItem() const { return curItem; }
signals:
    void listDown(PlayListItem *item, const QString &errInfo, const QList<QWebdavItem> davItems);
private:
    PlayListItem *curItem;
    QWebdavDirParser *webdavDirParser;
    QWebdav *webdavNetManager;
};

class PlayList : public QAbstractItemModel
{
    Q_OBJECT
    friend class PlayListPrivate;
public:
    explicit PlayList(QObject *parent = nullptr);
    ~PlayList();
    enum LoopMode
    {
        NO_Loop_One,
        NO_Loop_All,
        Loop_One,
        Loop_All,
        Random
    };
    enum ItemRole
    {
        BgmCollectionRole = Qt::UserRole+1,
        FolderCollectionRole,
        WebDAVCollectionRole,
        ColorMarkerRole,
        FilePathRole
    };

    const int maxRecentItems = 4;
    const PlayListItem *getCurrentItem() const;
    QModelIndex getCurrentIndex() const;
    const PlayListItem *getItem(const QModelIndex &index){return index.isValid()?static_cast<PlayListItem*>(index.internalPointer()):nullptr; }
    QList<const PlayListItem *> getSiblings(const PlayListItem *item, bool sameAnime=true);
    const PlayListItem *getItem(QModelIndex parent, const QStringList &path);
    LoopMode getLoopMode() const;
    bool canPaste() const;
    const QVector<RecentlyPlayedItem> &recent();
    void removeRecentItem(const QString &path);
    void setFinishTimeOnce(bool on);
    bool saveFinishTimeOnce();
    bool hasPath(const QString &path) const;
    bool isAutoMatch() const;
    QString matchFilters() const;
    bool isAddExternal() const;
signals:
    void currentInvaild();
    void currentMatchChanged(const QString &pid);
    void recentItemsUpdated();
    void recentItemInfoUpdated(int index);
    void matchStatusChanged(bool on);
public slots :
    int addItems(const QStringList &items, QModelIndex parent, const QHash<QString, QString> *titleMapping = nullptr);
    int addFolder(QString folderStr, QModelIndex parent, const QString &name = "");
    int addURL(const QStringList &urls, QModelIndex parent, bool decodeTitle = false, const QHash<QString, QString> *titleMapping = nullptr);
    QModelIndex addCollection(QModelIndex parent, const QString &title);
    QModelIndex getCollection(QModelIndex parent, const QStringList &path);
    int refreshFolder(const QModelIndex &index);
    QModelIndex addItem(QModelIndex parent, PlayListItem *item);
    QModelIndex addWebDAVCollection(QModelIndex parent, const QString &title, const QString &url, const QString &user, const QString &password);
    void refreshWebDAVCollection(const QModelIndex &index);

    void deleteItems(const QModelIndexList &deleteIndexes);
    int deleteInvalidItems(const QModelIndexList &indexes);
    void clear();

    void sortItems(const QModelIndex &parent,bool ascendingOrder);
    void sortAllItems(bool ascendingOrder);

    void cutItems(const QModelIndexList &cutIndexes);
    void pasteItems(QModelIndex parent);
    void moveUpDown(QModelIndex index,bool up);
    void switchBgmCollection(const QModelIndex &index);
    void setMarker(const QModelIndex &index, PlayListItem::Marker marker);
    void autoMoveToBgmCollection(const QModelIndex &index);

    const PlayListItem *setCurrentItem(const QModelIndex &index,bool playChild=true);
    const PlayListItem *setCurrentItem(const QString &path);
	void cleanCurrentItem();
    void setCurrentPlayTime();
    void addCurrentSub(const QString &subFile);
    void clearCurrentSub();
    void setCurrentSubDelay(int delay);
    void setCurrentSubIndex(int index);
    void addCurrentAudio(const QString &audioFile);
    void clearCurrentAudio();
    void setCurrentAudioIndex(int index);

    const PlayListItem *playPrevOrNext(bool prev);
    void setLoopMode(PlayList::LoopMode newMode);
    void checkCurrentItem(PlayListItem *itemDeleted);

    void setAutoMatch(bool on);
    void setAddExternal(bool on);
    void matchItems(const QModelIndexList &matchIndexes);
    void matchIndex(QModelIndex &index, const MatchResult &match);
    void matchItems(const QList<const PlayListItem *> &items, const QString &title, const QList<EpInfo> &eps);
    void removeMatch(const QModelIndexList &matchIndexes);
    void setMatchFilters(const QString &filters);

    void updateItemsDanmu(const QModelIndexList &itemIndexes);
    QModelIndex mergeItems(const QModelIndexList &mergeIndexes);
    void exportDanmuItems(const QModelIndexList &exportIndexes);

    
    void dumpJsonPlaylist(QJsonDocument &jsonDoc);
    QString getPathByHash(const QString &hash);
    const PlayListItem *getPathItem(const QString &pathId);
    void updatePlayTime(const QString &path, int time, PlayListItem::PlayState state);
    void renameItemPoolId(const QString &opid, const QString &npid);

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    inline virtual int columnCount(const QModelIndex &) const {return 1;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    inline virtual Qt::DropActions supportedDropActions() const{return Qt::MoveAction;}
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    inline virtual QStringList mimeTypes() const { return QStringList()<<"application/x-kikoplaylistitem"; }
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
private:
    PlayListPrivate * const d_ptr;
    MatchWorker *matchWorker;
    WebDAVWorker *webdavWorker;
    Q_DECLARE_PRIVATE(PlayList)
    Q_DISABLE_COPY(PlayList)

};
#endif // PLAYLIST_H
