#ifndef ANIMEWORKER_H
#define ANIMEWORKER_H
#include "animeinfo.h"
class AnimeWorker : public QObject
{
    Q_OBJECT
public:
    explicit AnimeWorker(QObject *parent=nullptr);
    ~AnimeWorker();

public:
    static AnimeWorker *instance()
    {
        static AnimeWorker worker;
        return &worker;
    }
    int fetchAnimes(QList<Anime *> *animes, int offset, int limit);
    void loadCrImages(Anime *anime);
    void loadEpInfo(Anime *anime);
    void loadPosters(Anime *anime);

    void addAnime(const MatchResult &match);
    void addAnime(const QString &name);
    void addAnime(Anime *anime);

    void addEp(const QString &animeName, const EpInfo &ep);



private:
    QMap<QString,Anime *> animesMap;

    QMultiMap<QString, QString> animeAlias;
    QMap<QString, QString> aliasAnime;
    bool aliasLoaded;
    void loadAlias();
    QString isAlias(const QString &name);

    bool checkAnimeExist(const QString &name);
    bool checkEpExist(const QString &animeName, const EpInfo &ep);

    void updateAnimeInfo(Anime *anime);
    QString downloadLabelInfo(Anime *anime);
    //QString isAlias(const QString &animeName);
    void setAlias(const QString &animeName, const QString &alias);

signals:
    void animeAdded(Anime *anime);
    void epRemoved(const QString &animeName, const QString &epPath);
    void epUpdated(const QString &animeName, const QString &epPath);
    void epAdded(const QString &animeName, const EpInfo &ep);

public slots:
    void loadAnimes(QList<Anime *> *animes, int offset, int limit);
    void loadLabelInfo(QMap<QString,QSet<QString> > &tagMap, QMap<QString, int> &timeMap);

    void updateCrtImage(const QString &title, const Character *crt);

    void addAnimeInfo(const QString &animeName, const QString &epName, const QString &path);
    void addAnimeInfo(const QString &animeName,int bgmId);
    void downloadDetailInfo(Anime *anime, int bangumiId);
    void downloadTags(int bangumiID, QStringList &tags);
    void saveTags(const QString &title, const QStringList &tags);

    void deleteAnime(Anime *anime);
    void updatePlayTime(const QString &title, const QString &path);
    void deleteTag(const QString &tag,const QString &animeTitle);
signals:
    void addAnime(Anime *anime);
    void mergeAnime(Anime *oldAnime,Anime *newAnime);
    void downloadDone(const QString &errInfo);
    void downloadTagDone(const QString &errInfo);
    void deleteDone();
    void loadDone(int count);
    void loadLabelInfoDone();
    void newTagDownloaded(const QString &animeTitle, const QStringList &tags);
    void downloadDetailMessage(const QString &msg);
};
#endif // ANIMEWORKER_H
