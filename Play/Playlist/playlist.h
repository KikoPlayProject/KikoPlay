#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <QObject>
#include <QAbstractItemModel>
#include <QModelIndex>
#include <QVariant>
#include <QDataStream>
#include <QSortFilterProxyModel>
#include <qDebug>

class QXmlStreamWriter;
class PlayList;
struct MatchInfo;
class PlayListItem
{
public:
    PlayListItem(PlayListItem *parent=nullptr,bool leaf=false,int insertPosition=-1);
    ~PlayListItem();
    void setLevel(int newLevel);
    void moveTo(PlayListItem *newParent, int insertPosition = -1);
    PlayListItem *parent;
    QString title;
    QString animeTitle;
    QString path;
    QString poolID;
    int playTime;
    QList<PlayListItem *> *children;
    int level;
    static PlayList *playlist;
};
class PlayList : public QAbstractItemModel
{
    Q_OBJECT
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
    const int maxRecentItems=4;
    const PlayListItem *getCurrentItem() const{return currentItem;}
    QModelIndex getCurrentIndex() const;
    inline const PlayListItem *getItem(const QModelIndex &index){return index.isValid()?static_cast<PlayListItem*>(index.internalPointer()):nullptr; }
    inline LoopMode getLoopMode() const{return loopMode;}
    inline bool canPaste() const {return itemsClipboard.count()>0;}
    QList<QPair<QString,QString> > &recent(){return recentList;}
private:
    PlayListItem root;   
    QList<PlayListItem *> itemsClipboard;
    QList<QPair<QString,QString> > recentList;
    PlayListItem *currentItem;
    bool playListChanged;
    LoopMode loopMode;
signals:
    void currentInvaild();
    void currentMatchChanged();
    void message(QString msg,int flag);
public slots :
    int addItems(QStringList &items, QModelIndex parent);
	void addFolder(QString folderStr, QModelIndex parent);
    void deleteItems(const QModelIndexList &deleteIndexes);
    void clear();
    void sortItems(const QModelIndex &parent,bool ascendingOrder);
    void sortAllItems(bool ascendingOrder);
    QModelIndex addCollection(QModelIndex parent,QString title);
    void cutItems(const QModelIndexList &cutIndexes);
    void pasteItems(QModelIndex parent);
    void moveUpDown(QModelIndex index,bool up);
    const PlayListItem *setCurrentItem(const QModelIndex &index,bool playChild=true);
    const PlayListItem *setCurrentItem(const QString &path);
	void cleanCurrentItem();
    const PlayListItem *playPrevOrNext(bool prev);
    void setLoopMode(LoopMode newMode){this->loopMode=newMode;}
    void checkCurrentItem(PlayListItem *itemDeleted);
    void matchItems(const QModelIndexList &matchIndexes);
    void matchCurrentItem(MatchInfo *matchInfo);
    void matchIndex(QModelIndex &index,MatchInfo *matchInfo);
    void setCurrentPlayTime(int playTime);
    QModelIndex mergeItems(const QModelIndexList &mergeIndexes);

    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const;
    virtual QModelIndex parent(const QModelIndex &child) const;
    virtual int rowCount(const QModelIndex &parent) const;
    inline virtual int columnCount(const QModelIndex &) const {return 1;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    inline virtual Qt::DropActions supportedDropActions() const{return Qt::MoveAction;}
    virtual Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QStringList mimeTypes() const;
    virtual QMimeData *mimeData(const QModelIndexList &indexes) const;
    virtual bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    virtual bool setData(const QModelIndex &index, const QVariant &value, int role);

private:
    void loadPlaylist();
    void loadRecentlist();
    void saveRecentlist();
    void savePlaylist();
    void updateRecentlist();
    void saveItem(QXmlStreamWriter &writer,PlayListItem *item);
    bool addSubFolder(QString folderStr, PlayListItem *parent,QSet<QString> &paths,int &itemCount);
    void addMediaItemPath(QSet<QString> &paths);
    PlayListItem *getPrevOrNextItem(LoopMode loopMode,bool prev);
    void updateItemInfo(PlayListItem *item);
    QString setCollectionTitle(QList<PlayListItem *> &list);
    void updateLibraryInfo(PlayListItem *item);

};
#endif // PLAYLIST_H
