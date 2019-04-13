#ifndef ANIMEWORKER_H
#define ANIMEWORKER_H
#include "animeinfo.h"
class AnimeWorker : public QObject
{
    Q_OBJECT
public:
    explicit AnimeWorker(QObject *parent=nullptr):QObject(parent){}
    ~AnimeWorker();
private:
    QMap<QString,Anime *> animesMap;
    void updateAnimeInfo(Anime *anime);
    QString downloadLabelInfo(Anime *anime);
    QString isAlias(const QString &animeName);
    void setAlias(const QString &animeName, const QString &alias);

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
