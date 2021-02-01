#ifndef ANIMEPROVIDER_H
#define ANIMEPROVIDER_H
#include <QObject>
#include "animeinfo.h"
#include "Script/scriptbase.h"
class AnimeProvider : public QObject
{
    Q_OBJECT
public:
    AnimeProvider(QObject *parent = nullptr);

    QList<QPair<QString, QString>> getSearchProviders();
    QList<QPair<QString, QString>> getMatchProviders();

    ScriptState animeSearch(const QString &scriptId, const QString &keyword, QList<AnimeBase> &results);
    ScriptState getDetail(const AnimeBase &base, Anime *anime);
    ScriptState getEp(Anime *anime, QList<EpInfo> &results);
    ScriptState getTags(Anime *anime, QStringList &results);

    ScriptState match(const QString &scriptId, const QString &path, MatchResult &result);
    ScriptState menuClick(const QString &mid, Anime *anime);
signals:
    void matchProviderChanged();
private:
    QSet<QString> matchProviderIds;
};

#endif // ANIMEPROVIDER_H
