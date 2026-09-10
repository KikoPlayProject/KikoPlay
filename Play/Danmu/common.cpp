#include "common.h"
#include "globalobjects.h"
#include "Render/danmurender.h"
#include <limits>
#ifdef KSERVICE
#include "Service/kservice.h"
#endif
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

DanmuTimeResult DanmuSource::mapTime(int rawOriginTimeMs) const
{
    const bool clipped = hasClip();
    const qint64 sourceTime = qint64(rawOriginTimeMs) - (clipped ? clipStart : 0);
    DanmuTimeResult result{sourceTime, sourceTime, DanmuTimeStatus::Visible};
    if (clipped && (rawOriginTimeMs < clipStart ||
                    qint64(rawOriginTimeMs) > qint64(clipStart) + clipDuration))
    {
        result.status = DanmuTimeStatus::OutsideClip;
        return result;
    }

    qint64 offset = delay;
    // Sorted rules apply strictly after their threshold, including equal and negative thresholds.
    for (const auto &rule : timelineInfo)
    {
        if (sourceTime <= rule.first) break;
        offset += rule.second;
    }
    result.finalTimeMs = sourceTime + offset;
    if (result.finalTimeMs < 0) result.status = DanmuTimeStatus::BeforeZero;
    else if (result.finalTimeMs > std::numeric_limits<int>::max())
        result.status = DanmuTimeStatus::OutOfRange;
    return result;
}

bool DanmuSource::hasClip() const
{
    return clipStart >= 0 && clipDuration > 0;
}

void DanmuSource::setClip(const QString &clipStr)
{
    if (clipStr.isEmpty()) return;
    const QStringList clipInfo = clipStr.split(':', Qt::SkipEmptyParts);
    if (clipInfo.size() == 2)
    {
        clipStart = clipInfo[0].toInt();
        clipDuration = clipInfo[1].toInt();
    }
}

void DanmuSource::setClip(int start, int duration)
{
    clipStart = start;
    clipDuration = duration;
}

QString DanmuSource::clipStr() const
{
    if (!hasClip()) return "";
    return QString("%1:%2").arg(clipStart).arg(clipDuration);
}

bool DanmuSource::isKikoSource() const
{
#ifdef KSERVICE
    return scriptId == KService::kSrc;
#endif
    return false;
}

bool DanmuSource::setTags(const QString &tagsJson)
{
    if (tagsJson.isEmpty()) return false;
    QJsonParseError jsonError;
    QJsonDocument document = QJsonDocument::fromJson(tagsJson.toUtf8(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError || !document.isArray())
    {
        return false;
    }
    QJsonArray tagArray = document.array();
    tags.clear();
    for (const auto &tagObj : tagArray)
    {
        if (tagObj.isObject())
        {
            DanmuSourceTag tag;
            tag.fromMap(tagObj.toObject().toVariantMap());
            tags.append(tag);
        }
    }
    return true;
}

QString DanmuSource::tagsJson() const
{
    if (tags.isEmpty()) return "";
    return QJsonDocument::fromVariant(packTags()).toJson(QJsonDocument::JsonFormat::Compact);
}

QDataStream &operator<<(QDataStream &stream, const DanmuSource &src)
{
    return stream << src.title
                  << src.desc
                  << src.scriptId
                  << src.scriptData
                  << src.id
                  << src.duration
                  << src.delay
                  << src.timelineStr()
                  << src.clipStr()
                  << src.tagsJson();
}

QDataStream &operator>>(QDataStream &stream, DanmuSource &src)
{
    QString timeline, clip, tagJson;
    stream >> src.title
           >> src.desc
           >> src.scriptId
           >> src.scriptData
           >> src.id
           >> src.duration
           >> src.delay
           >> timeline
           >> clip
           >> tagJson;
    if (!timeline.isEmpty()) src.setTimeline(timeline);
    if (!clip.isEmpty()) src.setClip(clip);
    if (!tagJson.isEmpty()) src.setTags(tagJson);
    return stream;
}

void DanmuSourceTag::fromMap(const QVariantMap &map)
{
    text = map.value("text").toString();
    textColor = map.value("text_color", -1).toInt();
    bgColor = map.value("bg_color", -1).toInt();
    tooltip = map.value("tooltip").toString();
    iconSVG = map.value("icon_svg").toString();
    link = map.value("link").toString();
    key = map.value("key").toString();
    data = map.value("data").toString();
}

QString formatTime(int mSec)
{
    int cs = (mSec >= 0 ? mSec : -mSec) / 1000;
    int cmin = cs/60;
    int cls = cs - cmin*60;
    return QString("%0%1:%2").arg(mSec < 0 ? "-" : "").arg(cmin, 2, 10,QChar('0')).arg(cls, 2, 10,QChar('0'));
}
