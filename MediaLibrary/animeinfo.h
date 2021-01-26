#ifndef ANIMEINFO_H
#define ANIMEINFO_H
#include <QtCore>
#include <QPixmap>

struct EpInfo
{
    enum class EpType
    {
        EP=1, SP, OP, ED, Trailer, MAD, Other
    };
    EpType type = EpType::EP;
    double index = 0;
    QString name;
    QString localFile;
    qint64 finishTime = 0, lastPlayTime = 0;
    QString toString() const
    {
        if(type == EpType::EP || type == EpType::SP)
            return QObject::tr("No.%0 %1").arg(index).arg(name);
        return name;
    }
    QVariantMap toMap() const { return {{"name", name}, {"index", index}, {"type", int(type)}, {"localFile", localFile}, {"finishTime", QString::number(finishTime)}, {"lastPlayTime", QString::number(lastPlayTime)}}; }
};
struct AnimeBase
{
    QString name;
    QString id;
    QSharedPointer<QList<EpInfo>> epList;
};
struct MatchResult
{
    bool success;
    QSharedPointer<AnimeBase> anime;
    QSharedPointer<EpInfo> ep;
};
struct Character
{
    QString name;
    QString id;
    QString actor;
    QString link;
    QString imgURL;
};
struct Anime
{
    QString name;
    QString desc;
    QString airDate;
    QString id;
    QString scriptId;
    qint64 addTime;
    int epCount;
    QString coverURL;
    QPixmap coverPixmap;

    QList<QPair<QString,QString>> staff;
    QList<EpInfo> epList;
    QList<Character> characters;
    QMap<QString, QByteArray> crtImages;
    QList<QByteArray> posters;

    QVariantMap toMap() const
    {
        QVariantList eps;
        for(const auto &ep : epList)
            eps.append(ep.toMap());
        return {{"name", name}, {"desc", desc}, {"id", id}, {"airDate", airDate}, {"epCount", epCount}, {"addTime", QString::number(addTime)}, {"eps", eps}};
    }
};

//struct Character
//{
//    QString name;
//    QString name_cn;
//    QString actor;
//    int bangumiID;
//    QString imgURL;
//    QByteArray image;
//};
//struct Episode
//{
//    QString name;
//    QString localFile;
//    QString lastPlayTime;
//};
//struct Anime
//{
//    QString title;
//    int epCount;
//    QString summary;
//    QString date;
//    QPixmap coverPixmap;
//    int bangumiID;
//    bool loadCrtImage;
//    qint64 addTime;
//    //QMap<QString,QString> staff;
//    QList<QPair<QString,QString> > staff;
//    //QByteArray cover;
//    QList<Episode> eps;
//    QList<Character> characters;
//};
struct CaptureItem
{
    qint64 timeId;
    QString info;
    QByteArray thumb;
};

#endif // ANIMEINFO_H
