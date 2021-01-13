#ifndef DANMUCOMMENT_H
#define DANMUCOMMENT_H
#include <QtCore>
#include <QtGui>
struct DanmuComment
{
    DanmuComment():time(0),originTime(0),blockBy(-1),mergedList(nullptr),m_parent(nullptr){}
    ~DanmuComment(){if(mergedList)delete mergedList;}

    enum DanmuType
    {
        Rolling,
        Top,
        Bottom,
        UNKNOW
    };
    enum FontSizeLevel
    {
        Normal,
        Small,
        Large
    };
    void setType(int mode)
    {
        switch (mode)
        {
        case 1:
        case 2:
        case 3:
            type=Rolling;
            break;
        case 4:
            type=Bottom;
            break;
        case 5:
            type=Top;
            break;
        default:
            type=Rolling;
            break;
        }
    }
    QString text;
    QString sender;
    int color;
    DanmuType type;
    FontSizeLevel fontSizeLevel;
    qint64 date;
    int time;
    int originTime;
    int blockBy;
    int source;

    QList<QSharedPointer<DanmuComment> > *mergedList;
    DanmuComment *m_parent;
    QVariantMap toMap() const {return {{"text", text}, {"time", originTime}, {"color", color}, {"fontsize", fontSizeLevel}, {"date", date}, {"type", type}};}
};
QDataStream &operator<<(QDataStream &stream, const DanmuComment &danmu);
QDataStream &operator>>(QDataStream &stream, DanmuComment &danmu);

Q_DECLARE_OPAQUE_POINTER(DanmuComment *)
struct SimpleDanmuInfo
{
    int time;
    int originTime;
    QString text;
};
class DanmuDrawInfo
{
public:
    int width;
    int height;
    int useCount;
    GLuint texture;
    GLfloat l,r,t,b;
    //QMutex useCountLock;
    //QImage *img=nullptr;
    //~DanmuDrawInfo(){if(img)delete img;}
};
class DanmuObject
{
private:
    static DanmuObject *head;
    static int poolCount;
    DanmuObject *next=nullptr;
public:   
    DanmuDrawInfo *drawInfo;
    QSharedPointer<DanmuComment> src;
    float x;
    float y;
    float extraData;
     ~DanmuObject();
    void *operator new(size_t sz);
    void  operator delete(void * p);
    static void DeleteObjPool();
};
struct MatchInfo
{
    bool success;
    bool error;
    QString errorInfo;
    QString poolID;
    QString fileHash;
    struct DetailInfo
    {
        QString animeTitle;
        QString title;
        //int episodeId;
    };
    QList<DetailInfo> matches;
};
QDataStream &operator<<(QDataStream &stream, const MatchInfo &match);
QDataStream &operator>>(QDataStream &stream, MatchInfo &match);
QDataStream &operator<<(QDataStream &stream, const MatchInfo::DetailInfo &md);
QDataStream &operator>>(QDataStream &stream, MatchInfo::DetailInfo &md);
struct DanmuSourceInfo
{
    int id;
    int delay;
    int count;
    QString name;
    QString url;
    bool show;
    QList<QPair<int,int> >timelineInfo;
    void setTimeline(const QString &timelineStr);
    QString getTimelineStr() const;
};
QDataStream &operator<<(QDataStream &stream, const DanmuSourceInfo &src);
QDataStream &operator>>(QDataStream &stream, DanmuSourceInfo &src);

struct BlockRule
{
    enum Field
    {
        DanmuText,
        DanmuColor,
        DanmuSender
    };
    enum Relation
    {
        Contain,
        Equal,
        NotEqual
    };
    int id;
    int blockCount;
    Field blockField;
    Relation relation;
    bool isRegExp;
    bool enable;
    bool usePreFilter;
    QString name;
    QString content;
    QScopedPointer<QRegExp> re;
    bool blockTest(DanmuComment *comment);
    BlockRule(const QString &ruleContent, Field field, Relation r);
    BlockRule() = default;
};
typedef QList<QPair<QSharedPointer<DanmuComment>,DanmuDrawInfo *> > PrepareList;
struct DanmuEvent
{
    int start;
    int duration;
    QString description;
};
struct DanmuSource
{
    // from script----
    QString title;
    QString desc;
    QString scriptData;
    QString scriptId;
    //---------
    int id;
    int delay;
    int count;
    int duration;
    bool show;

    QVariantMap toMap() const {return {{"title", title}, {"desc", desc}, {"data", scriptData}, {"duration", duration}, {"delay", delay}};}
};
Q_DECLARE_OPAQUE_POINTER(DanmuSource *)
struct SourceCollection
{
    QString errorInfo;
    QString providerId;
    QList<DanmuSource> list;
};

#endif // DANMUCOMMENT_H
