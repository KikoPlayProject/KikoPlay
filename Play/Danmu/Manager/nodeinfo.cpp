#include "nodeinfo.h"
#include "Extension/Script/scriptmanager.h"
#include "Extension/Script/danmuscript.h"
#include "globalobjects.h"
#include <QSvgRenderer>

DanmuPoolNode::DanmuPoolNode(DanmuPoolNode::NodeType nodeType,DanmuPoolNode *pNode):type(nodeType),danmuCount(0),
    checkStatus(Qt::Unchecked),parent(pNode),children(nullptr)
{
    if(type==NodeType::AnimeNode || type==NodeType::EpNode)
        children=new QList<DanmuPoolNode *>();
    if(parent)parent->children->append(this);
}

DanmuPoolNode::~DanmuPoolNode()
{
    if(children)
    {
        qDeleteAll(*children);
        delete children;
    }
}

int DanmuPoolNode::setCount()
{
    if(type==SourecNode)return danmuCount;
    int sum=0;
    for(DanmuPoolNode *child:*children)
    {
        sum+=child->setCount();
    }
    danmuCount=sum;
    return danmuCount;
}

void DanmuPoolNode::setChildrenCheckStatus()
{
    if(type==SourecNode)return;
    if(checkStatus!=Qt::PartiallyChecked)
    {
        for(DanmuPoolNode *child:*children)
        {
            child->checkStatus=checkStatus;
            child->setChildrenCheckStatus();
        }
    }
}

void DanmuPoolNode::setParentCheckStatus()
{
    if(parent)
    {
        int checkStatusStatis[3]={0,0,0};
        for(DanmuPoolNode *child:*parent->children)
        {
            checkStatusStatis[child->checkStatus]++;
        }
        //when there is only one source in the epNode, avoid removing the epNode
        if(checkStatusStatis[2]==parent->children->size() && parent->type!=NodeType::EpNode)
            parent->checkStatus=Qt::Checked;
        else if(checkStatusStatis[0]==parent->children->size())
            parent->checkStatus=Qt::Unchecked;
        else
            parent->checkStatus=Qt::PartiallyChecked;
        parent->setParentCheckStatus();
    }
}

int DanmuPoolNode::idHash(const QString &str)
{
    size_t hash = 0;
    for(int i=0;i<str.length();++i)
    {
        hash=hash * 131 + str.at(i).unicode();
    }
    return hash % 5;
}

DanmuPoolSourceNode::DanmuPoolSourceNode(const DanmuSource &src, DanmuPoolNode *pNode)
    : DanmuPoolNode(DanmuPoolNode::SourecNode, pNode)
{
    title = src.title;
    idInfo = src.scriptId;
    if (src.isKikoSource())
    {
        isKikoSrc = true;
        idInfo = "Kiko";
    }
    srcId = src.id;
    delay = src.delay;
    scriptData = src.scriptData;
    scriptSrcId = src.scriptSrcId;
    url = src.url;
    danmuCount = src.count;
    hasTimeline = !src.timelineInfo.isEmpty();
    hasClip = src.hasClip();
    valid = src.sourceValid;

    if (src.tags.isEmpty()) return;


    QColor tagBgColor{43, 106, 176};
    if (src.isKikoSource())
    {
        tagBgColor = QColor(19, 165, 200);
    }
    else if (!src.scriptId.isEmpty())
    {
        auto script = GlobalObjects::scriptManager->getScript(src.scriptId).dynamicCast<DanmuScript>();
        tagBgColor = script ? script->labelColor() : tagBgColor;
    }
    for (const DanmuSourceTag &tag : src.tags)
    {
        DanmuPoolSourceNodeTag srcTag;
        srcTag.textColor = tag.textColor == -1 ? QColor(Qt::white) : QColor(tag.textColor);
        srcTag.bgColor = tag.bgColor == -1 ? tagBgColor : QColor(tag.bgColor);
        srcTag.text = tag.text;
        srcTag.link = tag.link;
        srcTag.tooltip = tag.tooltip;
        if (!tag.iconSVG.isEmpty())
        {
            QSvgRenderer renderer(tag.iconSVG.toUtf8());
            if (renderer.isValid())
            {
                QPixmap pixmap(QSize(64, 64));
                pixmap.fill(Qt::transparent);

                QPainter painter(&pixmap);
                painter.setRenderHint(QPainter::Antialiasing);
                renderer.render(&painter);
                painter.end();

                srcTag.icon = QIcon(pixmap);
            }
        }
        tags.append(srcTag);
    }
}

bool DanmuPoolSourceNode::isSameSource(const DanmuSource &src) const
{
    if (src.scriptId != idInfo) return false;
    if (!src.scriptSrcId.isEmpty() && !scriptSrcId.isEmpty())
    {
        return src.scriptSrcId == scriptSrcId;
    }
    return scriptData == src.scriptData;
}

