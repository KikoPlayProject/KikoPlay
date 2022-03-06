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
    const QList<QPair<QString, QString>> &getMatchProviders() {return matchProviders;}
    const QString &defaultMatchScript() const {return defaultMatchScriptId;}
    void setDefaultMatchScript(const QString &scriptId);

    ScriptState animeSearch(const QString &scriptId, const QString &keyword, QList<AnimeLite> &results);
    ScriptState getDetail(const AnimeLite &base, Anime *anime);
    ScriptState getEp(Anime *anime, QVector<EpInfo> &results);
    ScriptState getTags(Anime *anime, QStringList &results);

    ScriptState match(const QString &scriptId, const QString &path, MatchResult &result);
    ScriptState menuClick(const QString &mid, Anime *anime);
signals:
    void matchProviderChanged();
    void defaultMacthProviderChanged(const QString &name, const QString &id);
private:
    QSet<QString> matchProviderIds;
    QList<QPair<QString, QString>> matchProviders;
    void  setMacthProviders();
    QString defaultMatchScriptId;
};

#endif // ANIMEPROVIDER_H
