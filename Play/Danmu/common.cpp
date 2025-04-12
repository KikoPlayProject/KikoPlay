#include "common.h"
#include "globalobjects.h"
#include "Render/danmurender.h"
#define MAX_POOL_COUNT 100
DanmuObject *DanmuObject::head=nullptr;
int DanmuObject::poolCount=0;

bool BlockRule::blockTest(DanmuComment *comment, bool updateCount)
{
    if(!enable)return false;
    QString *testStr(nullptr);
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
            if (re.isNull()) re.reset(new QRegularExpression(content));
            QRegularExpressionMatch match = re->match(*testStr);
            if (match.hasMatch())
            {
                testResult = match.capturedLength(0) == testStr->length();
            }
            //if(re->indexIn(*testStr)!=-1)
            //{
            //    testResult=re->matchedLength()==testStr->length()?true:false;
            //}
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
        if (isRegExp)
        {
            if(re.isNull()) re.reset(new QRegularExpression(content));
            //testResult=(re->indexIn(*testStr)!=-1)?true:false;
            testResult = re->match(*testStr).hasMatch();
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
    if(testResult && updateCount) ++blockCount;
    return testResult;
}

BlockRule::BlockRule(const QString &ruleContent, BlockRule::Field field, BlockRule::Relation r) :
    blockCount(0), blockField(field), relation(r), isRegExp(false), enable(true), usePreFilter(false), content(ruleContent)
{

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

QDataStream &operator<<(QDataStream &stream, const DanmuComment &danmu)
{
    stream<<danmu.originTime<<danmu.type<<danmu.fontSizeLevel<<danmu.color
         <<danmu.date<<danmu.sender<<danmu.source<<danmu.text;
    return stream;
}

QDataStream &operator>>(QDataStream &stream, DanmuComment &danmu)
{
    stream>>danmu.originTime;
    danmu.time=danmu.originTime;
    stream>>danmu.type>>danmu.fontSizeLevel;
    stream>>danmu.color>>danmu.date>>danmu.sender>>danmu.source>>danmu.text;
    return stream;
}

void DanmuSource::setTimeline(const QString &timelineStr)
{
    QStringList timelineList(timelineStr.split(';',Qt::SkipEmptyParts));
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

QString DanmuSource::timelineStr() const
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

QDataStream &operator<<(QDataStream &stream, const DanmuSource &src)
{
    return stream<<src.title<<src.desc<<src.scriptId<<src.scriptData<<src.id<<src.duration<<src.delay<<src.timelineStr();
}

QDataStream &operator>>(QDataStream &stream, DanmuSource &src)
{
    QString timeline;
    stream>>src.title>>src.desc>>src.scriptId>>src.scriptData>>src.id>>src.duration>>src.delay>>timeline;
    if(!timeline.isEmpty()) src.setTimeline(timeline);
    return stream;
}
