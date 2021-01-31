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
struct AnimeBase
{
    QString name;
    QString scriptData;
    QSharedPointer<QList<EpInfo>> epList;
};
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
};
struct AnimeImage
{
    enum class ImageType
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

    QString _name;
    QString _desc;
    QString _airDate;
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
    const QString &scriptId() const {return _scriptId;}
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
public:
    void addEp(const EpInfo &ep);
    void updateEpTime(const QString &path, qint64 time, bool isFinished);
    void removeEp(const QString &epPath);

public:
    QVariantMap toMap();

private:
    void assign(const Anime *anime);
    void setStaffs(const QString &staffStrs);
    QString staffToStr() const;
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
