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
    QString downloadLabelInfo(Anime *anime, QMap<QString,QSet<QString> > &tagMap);

public slots:
    void addAnimeInfo(const QString &animeName, const QString &epName, const QString &path);
    QString downloadDetailInfo(Anime *anime, int bangumiId, QMap<QString,QSet<QString> > &tagMap);
    void loadAnimes(QList<Anime *> *animes, int offset, int limit);
    void deleteAnime(Anime *anime);
    void updatePlayTime(const QString &title, const QString &path);
    void loadLabelInfo(QMap<QString,QSet<QString> > &tagMap, QSet<QString> &timeSet);
    void deleteTag(const QString &tag,const QString &animeTitle);
signals:
    void addAnime(Anime *anime);
    void mergeAnime(Anime *oldAnime,Anime *newAnime);
    void downloadDone();
    void loadDone(int count);
    void loadLabelInfoDone();
    void newTagDownloaded(const QStringList &tags);
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
    void deleteAnime(const QModelIndex &index);
    void refreshEpPlayTime(const QString &title, const QString &path);
    void addEp(Anime *anime,const QString &epName,const QString &path);
    void modifyEpPath(Episode &ep,const QString &newPath);
    void removeEp(Anime *anime,const QString &path);
    int getCount(int type);
    const QMap<QString,QSet<QString> > &animeTags() const{return tagsMap;}
    const QSet<QString> &animeTime() const{return timeSet;}
    void deleteTag(const QString &tag,const QString &animeTitle=QString());
    void addTag(Anime *anime,const QString &tag);

signals:
    void tryAddAnime(const QString &animeName,const QString &epName,const QString &path);
    void animeCountChanged();
    void addTags(const QStringList &tagList);
    void addTimeLabel(const QString &time);
    void refreshLabelInfo();
public slots:
    void addAnime(Anime *anime);
private:
    QList<Anime *> animes;
    QList<Anime *> tmpAnimes;
    QMap<QString,QSet<QString> > tagsMap;
    QSet<QString> timeSet;
    bool active;
    static AnimeWorker *animeWorker;
    const int limitCount=200;
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
    //void setTimeRange(int index);
    void addTime(const QString &time){timeFilterSet.insert(time);invalidateFilter();}
    void addTag(const QString &tag){tagFilterSet.insert(tag);invalidateFilter();}
    void removeTime(const QString &time){timeFilterSet.remove(time);invalidateFilter();}
    void removeTag(const QString &tag){if(tagFilterSet.contains(tag)){tagFilterSet.remove(tag);invalidateFilter();}}

private:
    int filterType;
    qint64 timeAfter;
    QSet<QString> timeFilterSet,tagFilterSet;
    // QSortFilterProxyModel interface
protected:
    virtual bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const;
};
#endif // ANIMELIBRARY_H
