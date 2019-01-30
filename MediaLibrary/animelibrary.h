#ifndef ANIMELIBRARY_H
#define ANIMELIBRARY_H
#include "animeinfo.h"
#include <QAbstractItemModel>
class AnimeWorker;
class AnimeModel;
class LabelModel;
class AnimeLibrary : public QObject
{
    Q_OBJECT
public:
    explicit AnimeLibrary(QObject *parent = nullptr);
    ~AnimeLibrary();
    void addToLibrary(const QString &animeName,const QString &epName, const QString &path);
    Anime *downloadDetailInfo(Anime *anime, int bangumiId, QString *errorInfo);
    void refreshEpPlayTime(const QString &title, const QString &path);
    QString updateCrtImage(Anime *anime, Character *crt);

    void loadTags(QMap<QString,QSet<QString> > &tagMap, QMap<QString, int> &timeMap);
    void deleteTag(const QString &tag,const QString &animeTitle=QString());
    void addTags(Anime *anime, const QStringList &tags);
    void saveTags(const QString &title, const QStringList &tags);
    void downloadTags(Anime *anime, QStringList &tagList, QString *errorInfo);

    void fetchAnimes(QList<Anime *> &animeList, int curOffset, int limitCount);
    void deleteAnime(Anime *anime);
    void fillAnimeInfo(Anime *anime);
    int getTotalAnimeCount();
signals:
    void addAnime(Anime *anime);
    void removeOldAnime(Anime *anime);
    void tryAddAnime(const QString &animeName,const QString &epName,const QString &path);
    void addTagsTo(const QString &title, const QStringList &tagList);
    void removeTagFrom(const QString &title, const QString &tag);
    void removeTags(const QString &animeTitle, const QString &time);
    void addTimeLabel(const QString &time);
    void downloadDetailMessage(const QString &msg);
private:
    AnimeWorker *animeWorker;
public:
    AnimeModel *animeModel;
    LabelModel *labelModel;
};
#endif // ANIMELIBRARY_H
