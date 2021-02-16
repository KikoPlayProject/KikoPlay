#ifndef ANIMEINFO_H
#define ANIMEINFO_H
#include <QtCore>
#include <QPixmap>
enum EpType
{
    UNKNOWN, EP, SP, OP, ED, Trailer, MAD, Other
};
static const QString EpTypeName[] = {QObject::tr("EP"),QObject::tr("SP"),QObject::tr("OP"),QObject::tr("ED"),QObject::tr("Trailer"),QObject::tr("MAD"),QObject::tr("Other")};
struct EpInfo
{

    EpType type = EpType::UNKNOWN;  //UNKNOWN: Invalid EpInfo
    double index = 0;
    QString name;
    QString localFile;
    qint64 finishTime = 0, lastPlayTime = 0;

    EpInfo():type(EpType::UNKNOWN),index(0.0),finishTime(0),lastPlayTime(0) {}
    EpInfo(EpType epType, double epIndex, const QString &epName=""):type(epType),index(epIndex),name(epName),finishTime(0),lastPlayTime(0) {}

    bool operator <(const EpInfo &ep) const
    {
        if(type!=ep.type) return type<ep.type;
        return index<ep.index;
    }
    void infoAssign(const EpInfo &ep)
    {
        type = ep.type;
        name = ep.name;
        index= ep.index;
    }
    QString toString() const
    {
        if(type != EpType::UNKNOWN)
            return QObject::tr("%0.%1 %2").arg(EpTypeName[type-1]).arg(index).arg(name);
        return name;
    }
    QString playTimeStr(bool finish=false) const
    {
        qint64 time = finish?finishTime:lastPlayTime;
        return time==0?"":QDateTime::fromSecsSinceEpoch(time).toString("yyyy-MM-dd hh:mm:ss");
    }
    QVariantMap toMap() const
    {
        return
        {
            {"name", name},
            {"index", index},
            {"type", int(type)},
            {"localFile", localFile},
            {"finishTime", QString::number(finishTime)},
            {"lastPlayTime", QString::number(lastPlayTime)}
        };
    }
};
bool operator==(const EpInfo &ep1, const EpInfo &ep2);
bool operator!=(const EpInfo &ep1, const EpInfo &ep2);
Q_DECLARE_METATYPE(EpInfo)


class Anime;
struct AnimeLite
{
    QString name;
    QString extras;
    QString scriptId;
    QString scriptData;
    QSharedPointer<QList<EpInfo>> epList;
    Anime* toAnime() const;
    QVariantMap toMap() const
    {
        return
        {
            {"name", name},
            {"scriptData", scriptData},
        };
    }
};
Q_DECLARE_METATYPE(AnimeLite)

struct MatchResult
{
    bool success;
    QString name;
    QString scriptId;
    QString scriptData;
    EpInfo ep;
};


struct Character
{
    QString name;
    QString actor;
    QString link;
    QString imgURL;
    QPixmap image;
    QVariantMap toMap() const
    {
        return
        {
            {"name", name},
            {"actor", actor},
            {"link", link},
            {"imgurl", imgURL}
        };
    }
};
struct AnimeImage
{
    static const int thumbH = 112, thumbW = 200;
    enum ImageType
    {
        CAPTURE, SLICE, POSTER
    };
    ImageType type;
    qint64 timeId;
    QString info;
    QPixmap thumb;
};
class Anime
{
    friend class AnimeWorker;
    friend class LibraryScript;
    friend struct AnimeLite;
    friend class AnimeProvider;

    QString _name;
    QString _desc;
    QString _airDate;
    QString _url;
    QString _coverURL;
    QPixmap _cover;
    QString _scriptId;
    QString _scriptData;

    qint64 _addTime;
    int _epCount;


    QList<QPair<QString,QString>> staff;
    QList<EpInfo> epInfoList;
    QList<Character> characters;
    QList<AnimeImage> posters;

    bool crtImagesLoaded;
    bool epLoaded;
    bool posterLoaded;

public:
    const QString &name() const {return _name;}
    const QString &description() const {return _desc;}
    const QString &airDate() const {return _airDate;}
    const QString &url() const {return _url;}
    const QString &scriptId() const {return _scriptId;}
    int epCount() const {return  _epCount;}
    const QString &scriptData() const {return _scriptData;}
    qint64 addTime() const {return _addTime;}
    QString addTimeStr() const {return QDateTime::fromSecsSinceEpoch(_addTime).toString("yyyy-MM-dd hh:mm:ss");}
    const QPixmap &cover() const {return _cover;}
    const QString &coverURL() const {return _coverURL;}

public:
    Anime();
    bool isValid() const {return !_name.isEmpty();}

    const QList<EpInfo> &epList();
    const QList<Character> &crList(bool loadImage = false);
    const QStringList &tagList();
    const QList<AnimeImage> &posterList();
    const QList<QPair<QString,QString>> &staffList() const {return staff;}
private:
    void addEp(const EpInfo &ep);
    void updateEpTime(const QString &path, qint64 time, bool isFinished);
    void updateEpInfo(const QString &path, const EpInfo &nInfo);
    void updateEpPath(const QString &path, const QString &nPath);
    void removeEp(const QString &epPath);
    void removeEp(EpType type, double index);
    void removePoster(qint64 timeId);

public:
    QVariantMap toMap(bool fillEp = false);
    const AnimeLite toLite() const;

private:
    void assign(const Anime *anime);
    void setStaffs(const QString &staffStrs);
    QString staffToStr() const;
};
enum TagType
{
    TAG_SCRIPT, TAG_TIME, TAG_FILE, TAG_CUSTOM
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


#endif // ANIMEINFO_H
