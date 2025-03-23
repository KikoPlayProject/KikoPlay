#ifndef ANIMEINFO_H
#define ANIMEINFO_H
#include <QtCore>
#include <QPixmap>
namespace Extension
{
    class LibraryInterface;
}
enum EpType
{
    UNKNOWN, EP, SP, OP, ED, Trailer, MAD, Other
};
static const QString EpTypeName[] = {"EP", "SP","OP","ED","Trailer","MAD","Other"};
struct EpInfo
{

    EpType type = EpType::UNKNOWN;  //UNKNOWN: Invalid EpInfo
    double index = 0;
    QString name;
    QString localFile;
    qint64 finishTime = 0, lastPlayTime = 0;

    EpInfo():type(EpType::UNKNOWN),index(0.0),finishTime(0),lastPlayTime(0) {}
    EpInfo(EpType epType, double epIndex, const QString &epName=""):type(epType),index(epIndex),name(epName),finishTime(0),lastPlayTime(0) {}

    bool isValid() const
    {
        return type != EpType::UNKNOWN;
    }
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
    QSharedPointer<QVector<EpInfo>> epList;
    Anime* toAnime() const;
    QVariantMap toMap() const
    {
        return
        {
            {"name", name},
            {"scriptId", scriptId},
            {"data", scriptData},
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

    static void scale(QPixmap &img);
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
        CAPTURE, SNIPPET
    };
    ImageType type;
    qint64 timeId;
    QString info;
    QPixmap thumb;
    void setRoundedThumb(const QImage &src);
};
class Anime
{
    friend class AnimeWorker;
    friend class LibraryScript;
    friend struct AnimeLite;
    friend class AnimeProvider;
    friend class Extension::LibraryInterface;

    QString _name;
    QString _desc;
    QString _airDate;
    QString _url;
    QString _coverURL;
    QByteArray _coverData;
    QString _scriptId;
    QString _scriptData;
    qint64 _addTime;

    QVector<QPair<QString,QString>> staff;
    QVector<EpInfo> epInfoList;
    QVector<Character> characters;

    int _epCount;
    bool crtImagesLoaded;
    bool epLoaded;

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
    const QPixmap &cover(bool onlyCache=false);
    const QPixmap &rawCover();
    const QString &coverURL() const {return _coverURL;}

    constexpr static const char *emptyCoverURL = "<empty>";

public:
    Anime();
    bool isValid() const {return !_name.isEmpty();}

    void setAirDate(const QString &newAirDate);
    void setStaffs(const QVector<QPair<QString,QString>> &staffs);
    void setDesc(const QString &desc);
    void setEpCount(int epCount);
    void setCover(const QByteArray &data, bool updateDB=true, const QString &coverURL=emptyCoverURL);

    void setCrtImage(const QString &name, const QByteArray &data);
    void addCharacter(const Character &newCrtInfo);
    void removeCharacter(const QString &name);
    void modifyCharacterInfo(const QString &srcName, const Character &newCrtInfo);

    const QVector<EpInfo> &epList();
    const QVector<Character> &crList(bool loadImage = false);
    const QStringList &tagList();
    const QVector<QPair<QString,QString>> &staffList() const {return staff;}
private:
    void addEp(const EpInfo &ep);
    void updateEpTime(const QString &path, qint64 time, bool isFinished);
    void updateEpInfo(const QString &path, const EpInfo &nInfo);
    void updateEpPath(const QString &path, const QString &nPath);
    void removeEp(const QString &epPath);
    void removeEp(EpType type, double index);

public:
    QVariantMap toMap(bool fillEp = false);
    const AnimeLite toLite() const;
    static Anime *fromMap(const QVariantMap &aobj);

private:
    void assign(const Anime *anime);
    void setStaffs(const QString &staffStrs);
    QString staffToStr() const;
    static QString staffListToStr(const QVector<QPair<QString,QString>> &staffs);
};

struct AnimeInfoTag
{
    QMap<QString, int> airDateCount;
    QMap<QString, int> scriptIdCount;
    void clear()
    {
        airDateCount.clear();
        scriptIdCount.clear();
    }
};

#endif // ANIMEINFO_H
