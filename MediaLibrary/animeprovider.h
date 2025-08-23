#ifndef ANIMEPROVIDER_H
#define ANIMEPROVIDER_H
#include <QObject>
#include "animeinfo.h"
#include "Extension/Script/scriptbase.h"
class MatchStatusObj : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;
    void quitEventLoop() { emit quit(); }
public:
    std::atomic_bool downFlag{false};
    bool scriptValid{false};
    bool kServiceSuccess{false};
    bool scriptSuccess{false};
    MatchResult kServiceMatch;
    MatchResult scriptMatch;
signals:
    void quit();
};

class AnimeProvider : public QObject
{
    Q_OBJECT
public:
    AnimeProvider(QObject *parent = nullptr);

    QList<QPair<QString, QString>> getSearchProviders();
    const QList<QPair<QString, QString>> &getMatchProviders() {return matchProviders;}
    const QString &defaultMatchScript() const {return defaultMatchScriptId;}
    void setDefaultMatchScript(const QString &scriptId);

    ScriptState animeSearch(const QString &scriptId, const QString &keyword, const QMap<QString, QString> &options, QList<AnimeLite> &results);
    ScriptState getDetail(const AnimeLite &base, Anime *anime);
    ScriptState getEp(Anime *anime, QVector<EpInfo> &results);
    ScriptState getTags(Anime *anime, QStringList &results);

    ScriptState match(const QString &scriptId, const QString &path, MatchResult &result);
    ScriptState menuClick(const QString &mid, Anime *anime);
signals:
    void infoProviderChanged();
    void defaultMacthProviderChanged(const QString &name, const QString &id);
private:
    QSet<QString> matchProviderIds;
    QList<QPair<QString, QString>> matchProviders;
    void setMatchProviders();
    QString defaultMatchScriptId;
#ifdef KSERVICE
    ScriptState kMatch(const QString &scriptId, const QString &path, MatchResult &result);
#endif
};

#endif // ANIMEPROVIDER_H
