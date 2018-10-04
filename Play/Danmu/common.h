#ifndef DANMUCOMMENT_H
#define DANMUCOMMENT_H
#include <QtCore>
#include <QtGui>
class DanmuComment
{
public:
    DanmuComment():time(0),originTime(0),blockBy(-1){}
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
            type=UNKNOW;
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
};
Q_DECLARE_OPAQUE_POINTER(DanmuComment *)
class DanmuDrawInfo
{
public:
    int width;
    int height;
    int useCount;
    QMutex useCountLock;
    QImage *img=nullptr;
    ~DanmuDrawInfo(){if(img)delete img;}
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
     ~DanmuObject(){drawInfo->useCountLock.lock(); drawInfo->useCount--; drawInfo->useCountLock.unlock();}
    void *operator new(size_t sz);
    void  operator delete(void * p);
    static void DeleteObjPool();
};
struct DanmuSourceInfo
{
    int id;
    int delay;
    int count;
    QString name;
    QString url;
    bool show;
    QList<QPair<int,int> >timelineInfo;
};
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
    Field blockField;
    Relation relation;
    bool isRegExp;
    bool enable;
    QString content;
    bool blockTest(DanmuComment *comment);
};
typedef QList<QPair<QSharedPointer<DanmuComment>,DanmuDrawInfo *> > PrepareList;
#endif // DANMUCOMMENT_H
