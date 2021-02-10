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
    int animeCount();
    void loadCrImages(Anime *anime);
    void loadEpInfo(Anime *anime);
    void loadPosters(Anime *anime);

    void addAnime(const MatchResult &match);
    void addAnime(const QString &name);
    bool addAnime(Anime *anime);
    void addAnime(Anime *srcAnime, Anime *newAnime);
    void deleteAnime(Anime *anime);

    void addEp(const QString &animeName, const EpInfo &ep);
    void removeEp(const QString &animeName, const QString &path);
    void updateEpTime(const QString &animeName, const QString &path, bool finished = false, qint64 epTime=0);
    void updateEpInfo(const QString &animeName, const QString &path, const EpInfo &nEp);
    void updateEpPath(const QString &animeName, const QString &path, const QString &nPath);

    void saveCrtImage(const QString &animeName, const QString &crtName, const QByteArray &imageContent);
    void saveCapture(const QString &animeName, const QString &info, const QImage &image);
    const QPixmap getAnimeImageData(const QString &animeName, AnimeImage::ImageType type, qint64 timeId);
    void deleteAnimeImage(const QString &animeName, AnimeImage::ImageType type, qint64 timeId);
    int fetchCaptures(const QString &animeName, QList<AnimeImage> &captureList, int offset, int limit);

    void loadTags(QMap<QString,QSet<QString> > &tagMap, QMap<QString, int> &timeMap);
    void deleteTag(const QString &tag,const QString &animeTitle="");
    void addTags(const QString &aniemName, const QStringList &tags);
    void saveTags(const QString &animeName, const QStringList &tags);

private:
    QMap<QString,Anime *> animesMap;

    QMultiMap<QString, QString> animeAlias;
    QMap<QString, QString> aliasAnime;
    bool aliasLoaded;
    void loadAlias();
    QString isAlias(const QString &name);
    void addAlias(const QString &name, const QString &alias);
    void removeAlias(const QString &name, const QString &alias = "");

    bool checkAnimeExist(const QString &name);
    bool checkEpExist(const QString &animeName, const EpInfo &ep);

    bool updateAnimeInfo(Anime *anime);

    //QString downloadLabelInfo(Anime *anime);
    //QString isAlias(const QString &animeName);

signals:
    void animeAdded(Anime *anime);
    void animeUpdated(Anime *anime);
    void animeRemoved(Anime *anime);

    void epRemoved(const QString &animeName, const QString &epPath);
    void epUpdated(const QString &animeName, const QString &epPath);
    void epAdded(const QString &animeName, const EpInfo &ep);
    void epReset(const QString &animeName);

    void captureUpdated(const QString &animeName, const AnimeImage &aImage);

    void addTagsTo(const QString &animeName, const QStringList &tagList);
    void removeTagFrom(const QString &animeName, const QString &tag);
    void removeTags(const QString &animeTitle, const QString &time);
    void addTimeLabel(const QString &time, const QString &oldTime);


public slots:
    //void loadAnimes(QList<Anime *> *animes, int offset, int limit);
    //void loadLabelInfo(QMap<QString,QSet<QString> > &tagMap, QMap<QString, int> &timeMap);

    //void updateCrtImage(const QString &title, const Character *crt);

    //void addAnimeInfo(const QString &animeName, const QString &epName, const QString &path);
    //void addAnimeInfo(const QString &animeName,int bgmId);
    //void downloadDetailInfo(Anime *anime, int bangumiId);
    //void downloadTags(int bangumiID, QStringList &tags);
    //void saveTags(const QString &title, const QStringList &tags);

    //void deleteAnime(Anime *anime);
    //void updatePlayTime(const QString &title, const QString &path);
    //void deleteTag(const QString &tag,const QString &animeTitle);
//signals:
//    void addAnime(Anime *anime);
//    void mergeAnime(Anime *oldAnime,Anime *newAnime);
//    void downloadDone(const QString &errInfo);
//    void downloadTagDone(const QString &errInfo);
//    void deleteDone();
//    void loadDone(int count);
//    void loadLabelInfoDone();
//    void newTagDownloaded(const QString &animeTitle, const QStringList &tags);
//    void downloadDetailMessage(const QString &msg);
};
#endif // ANIMEWORKER_H
