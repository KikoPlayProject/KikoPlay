#ifndef ANIMELIBRARY_H
#define ANIMELIBRARY_H
#include "animeinfo.h"
#include <QAbstractItemModel>
class AnimeWorker : public QObject
{
    Q_OBJECT
public:
    explicit AnimeWorker(QObject *parent=nullptr):QObject(parent){}
private:
    QMap<QString,Anime *> animesMap;
    void updateAnimeInfo(Anime *anime);

public slots:
    void addAnimeInfo(const QString &animeName, const QString &epName, const QString &path);
    QString downloadDetailInfo(Anime *anime,int bangumiId);
    void loadAnimes(QList<Anime *> *animes, int offset, int limit);
    void deleteAnime(Anime *anime);
    void updatePlayTime(const QString &title, const QString &path);
signals:
    void addAnime(Anime *anime);
    void mergeAnime(Anime *oldAnime,Anime *newAnime);
    void downloadDone();
    void loadDone(int count);
};
class AnimeLibrary : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit AnimeLibrary(QObject *parent = nullptr);
    ~AnimeLibrary();
    void setActive(bool a);
    void addToLibrary(const QString &animeName,const QString &epName, const QString &path);
    Anime *getAnime(const QModelIndex &index,bool fillInfo=true);
    Anime *downloadDetailInfo(Anime *anime, int bangumiId, QString *errorInfo);
    void deleteAnime(QModelIndexList &deleteIndexes);
    void refreshEpPlayTime(const QString &title, const QString &path);
    void addEp(Anime *anime,const QString &epName,const QString &path);
    void modifyEpPath(Episode &ep,const QString &newPath);
    void removeEp(Anime *anime,const QString &path);
    int getCount(int type);

signals:
    void tryAddAnime(const QString &animeName,const QString &epName,const QString &path);
    void animeCountChanged();
public slots:
    void addAnime(Anime *anime);
private:
    QList<Anime *> animes;
    QList<Anime *> tmpAnimes;
    bool active;
    static AnimeWorker *animeWorker;
    const int limitCount=20;
    int currentOffset;
    bool hasMoreAnimes;
    // QAbstractItemModel interface
public:
    virtual QModelIndex index(int row, int column, const QModelIndex &parent) const{return parent.isValid()?QModelIndex():createIndex(row,column);}
    virtual QModelIndex parent(const QModelIndex &) const{return QModelIndex();}
    virtual int rowCount(const QModelIndex &parent) const {return parent.isValid()?0:animes.count();}
    virtual int columnCount(const QModelIndex &parent) const {return parent.isValid()?0:1;}
    virtual QVariant data(const QModelIndex &index, int role) const;
    virtual void fetchMore(const QModelIndex &parent);
    inline virtual bool canFetchMore(const QModelIndex &) const{ return hasMoreAnimes; }
    QList<Anime *> getAnimes() const;
    void setAnimes(const QList<Anime *> &value);
};
class AnimeFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
public:
    explicit AnimeFilterProxyModel(QObject *parent = nullptr):QSortFilterProxyModel(parent),filterType(0){}
    void setFilterType(int type){filterType=type;}
    void setTimeRange(int index);

private:
    int filterType;
    qint64 timeAfter;
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};
#endif // ANIMELIBRARY_H
