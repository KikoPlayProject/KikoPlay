#include "common.h"
#include "globalobjects.h"
#include "Render/danmurender.h"
#define MAX_POOL_COUNT 100
DanmuObject *DanmuObject::head=nullptr;
int DanmuObject::poolCount=0;

bool BlockRule::blockTest(DanmuComment *comment)
{
    if(!enable)return false;
    QString *testStr;
    bool testResult(false);
    switch (blockField)
    {
    case DanmuText:
        testStr=&comment->text;
        break;
    case DanmuSender:
        testStr=&comment->sender;
        break;
    case DanmuColor:
        testStr=new QString(QString::number(comment->color,16));
        break;
    }
    switch (relation)
    {
    case Equal:
    case NotEqual:
    {
        if(isRegExp)
        {
            if(re.isNull())re.reset(new QRegExp(content));
            if(re->indexIn(*testStr)!=-1)
            {
                testResult=re->matchedLength()==testStr->length()?true:false;
            }
        }
        else
        {
            testResult=(*testStr==content);
        }
        if(relation==Relation::NotEqual)testResult=!testResult;
    }
        break;
    case Contain:
    {
        if(isRegExp)
        {
            if(re.isNull())re.reset(new QRegExp(content));
            testResult=(re->indexIn(*testStr)!=-1)?true:false;
        }
        else
        {
            testResult=testStr->contains(content);
        }
    }
        break;
    default:
        break;
    }
    if(blockField==DanmuColor)delete testStr;
    return testResult;
}

DanmuObject::~DanmuObject()
{
    GlobalObjects::danmuRender->refDesc(drawInfo);
}

void *DanmuObject::operator new(size_t sz)
{
    if(head)
    {
        void *tmp=(void *)head;
        head=head->next;
        poolCount--;
        return tmp;
    }
    else
    {
        void *tmp=malloc(sz);
        if(!tmp) throw std::bad_alloc();
        return tmp;
    }
}

void DanmuObject::operator delete(void *p)
{
    if(poolCount>MAX_POOL_COUNT)
    {
        free(p);
    }
    else
    {
        DanmuObject *obj=static_cast<DanmuObject *>(p);
        obj->next=head;
        head=obj;
        poolCount++;
    }
}

void DanmuObject::DeleteObjPool()
{
    DanmuObject *p;
    for(p=head;p!=nullptr;p=head)
    {
        head=head->next;
        free((void *)p);
    }
    poolCount=0;
}

void DanmuSourceInfo::setTimeline(const QString &timelineStr)
{
    QStringList timelineList(timelineStr.split(';',QString::SkipEmptyParts));
    QTextStream ts;
    timelineInfo.clear();
    for(QString &spaceInfo:timelineList)
    {
        ts.setString(&spaceInfo,QIODevice::ReadOnly);
        int start,duration;
        ts>>start>>duration;
        timelineInfo.append(QPair<int,int>(start,duration));
    }
    std::sort(timelineInfo.begin(),timelineInfo.end(),[](const QPair<int,int> &s1,const QPair<int,int> &s2){
        return s1.first<s2.first;
    });
}

QString DanmuSourceInfo::getTimelineStr() const
{
    QString timelineStr;
    QTextStream ts(&timelineStr);
    for(auto &spaceItem:timelineInfo)
    {
        ts<<spaceItem.first<<' '<<spaceItem.second<<';';
    }
    ts.flush();
    return timelineStr;
}

QDataStream &operator<<(QDataStream &stream, const DanmuComment &danmu)
{
    static int type[3]={1,5,4};
    static int fontSize[3]={25,18,36};
    stream<<danmu.originTime<<type[danmu.type]<<fontSize[danmu.fontSizeLevel]<<danmu.color
         <<danmu.date<<danmu.sender<<danmu.source<<danmu.text;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, DanmuComment &danmu)
{
    stream>>danmu.originTime;
    danmu.time=danmu.originTime;
    int type,fontSize;
    stream>>type>>fontSize;
    danmu.setType(type);

    if(fontSize==18) danmu.fontSizeLevel=DanmuComment::Small;
    else if(fontSize==36) danmu.fontSizeLevel=DanmuComment::Large;
    else danmu.fontSizeLevel=DanmuComment::Normal;

    stream>>danmu.color>>danmu.date>>danmu.sender>>danmu.source>>danmu.text;
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const DanmuSourceInfo &src)
{
    stream<<src.id<<src.name<<src.url<<src.delay<<src.getTimelineStr();
    return stream;
}

QDataStream &operator>>(QDataStream &stream, DanmuSourceInfo &src)
{
    QString timeline;
    stream>>src.id>>src.name>>src.url>>src.delay>>timeline;
    if(!timeline.isEmpty()) src.setTimeline(timeline);
    return stream;
}

QDataStream &operator<<(QDataStream &stream, const MatchInfo &match)
{
    return stream<<match.success<<match.error<<match.errorInfo<<match.poolID<<match.fileHash<<match.matches;
}

QDataStream &operator>>(QDataStream &stream, MatchInfo &match)
{
    return stream>>match.success>>match.error>>match.errorInfo>>match.poolID>>match.fileHash>>match.matches;
}

QDataStream &operator<<(QDataStream &stream, const MatchInfo::DetailInfo &md)
{
    return stream<<md.title<<md.animeTitle;
}

QDataStream &operator>>(QDataStream &stream, MatchInfo::DetailInfo &md)
{
    return stream>>md.title>>md.animeTitle;
}
