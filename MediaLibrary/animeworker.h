#ifndef ANIMEWORKER_H
#define ANIMEWORKER_H
#include "animeinfo.h"
#include "tagnode.h"
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

    void addAnime(const MatchResult &match);
    void addAnime(const QString &name);
    bool addAnime(Anime *anime);
    const QString addAnime(Anime *srcAnime, Anime *newAnime);
    void deleteAnime(Anime *anime);
    Anime *getAnime(const QString &name);
    bool hasAnime(const QString &name);

    void addEp(const QString &animeName, const EpInfo &ep);
    void removeEp(const QString &animeName, const QString &path);
    void updateEpTime(const QString &animeName, const QString &path, bool finished = false, qint64 epTime=0);
    void updateEpInfo(const QString &animeName, const QString &path, const EpInfo &nEp);
    void updateEpPath(const QString &animeName, const QString &path, const QString &nPath);
    void updateCaptureInfo(const QString &animeName, qint64 timeId, const QString &newInfo);

    void updateCoverImage(const QString &animeName, const QByteArray &imageContent, bool resetCoverURL=false);
    void updateCrtImage(const QString &animeName, const QString &crtName, const QByteArray &imageContent);
    void saveCapture(const QString &animeName, const QString &info, const QImage &image);
    void saveSnippet(const QString &animeName, const QString &info, qint64 timeId, const QImage &image);
    const QPixmap getAnimeImageData(const QString &animeName, AnimeImage::ImageType type, qint64 timeId);
    void deleteAnimeImage(const QString &animeName, AnimeImage::ImageType type, qint64 timeId);
    int fetchCaptures(const QString &animeName, QList<AnimeImage> &captureList, int offset, int limit);

    void loadAnimeInfoTag(AnimeInfoTag &animeInfoTags);
    void loadEpInfoTag(QMap<QString, QSet<QString>> &epPathAnimes);
    void loadCustomTags(QMap<QString, QSet<QString> > &tagAnimes);

    void saveTags(const QString &animeName, const QStringList &tags);
    void deleteTag(const QString &tag,const QString &animeTitle="");
    void deleteTags(const QStringList &tags);

    bool addAlias(const QString &name, const QString &alias);
    void removeAlias(const QString &name, const QString &alias = "", bool updateDB=false);
    const QStringList getAlias(const QString &animeName);

private:
    QMap<QString,Anime *> animesMap;

    QMultiMap<QString, QString> animeAlias;
    QMap<QString, QString> aliasAnime;
    bool aliasLoaded;
    void loadAlias();
    QString isAlias(const QString &name);

    bool checkAnimeExist(const QString &name);
    bool checkEpExist(const QString &animeName, const EpInfo &ep);

    bool updateAnimeInfo(Anime *anime);

signals:
    void animeAdded(Anime *anime);
    void animeUpdated(Anime *anime);
    void animeRemoved(Anime *anime);

    void epRemoved(const QString &animeName, const QString &epPath);
    void epUpdated(const QString &animeName, const QString &epPath);
    void epAdded(const QString &animeName, const EpInfo &ep);
    void epReset(const QString &animeName);

    void captureUpdated(const QString &animeName, const AnimeImage &aImage);

    void renameEpTag(const QString &oldAnimeName, const QString &newAnimeName);
    void addTimeTag(const QString &tag);
    void addScriptTag(const QString &scriptId);
    void removeTimeTag(const QString &tag);
    void removeScriptTag(const QString &scriptId);
};
#endif // ANIMEWORKER_H
