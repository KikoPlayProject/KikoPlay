#ifndef NODEINFO_H
#define NODEINFO_H
#include <QtCore>
#include "../common.h"
#include "MediaLibrary/animeinfo.h"
struct DanmuPoolNode
{
    enum NodeType
    {
        AnimeNode,
        EpNode,
        SourecNode
    }; 
    QString title;
    QString idInfo; //empty in AnimeNode
    NodeType type;
    int danmuCount;
    int checkStatus;

    DanmuPoolNode *parent;
    QList<DanmuPoolNode *> *children;

    DanmuPoolNode(NodeType nodeType, DanmuPoolNode *pNode = nullptr);
    virtual ~DanmuPoolNode();
    int setCount();
    void setChildrenCheckStatus();
    void setParentCheckStatus();
    static int idHash(const QString &str);
};
struct DanmuPoolEpNode : public DanmuPoolNode
{
    DanmuPoolEpNode(DanmuPoolNode *pNode = nullptr):DanmuPoolNode(DanmuPoolNode::EpNode, pNode), epType(EpType::UNKNOWN), epIndex(0){}
    DanmuPoolEpNode(const EpInfo &ep, DanmuPoolNode *pNode = nullptr):DanmuPoolNode(DanmuPoolNode::EpNode, pNode), epName(ep.name), epType(ep.type), epIndex(ep.index)
    {
        title=ep.toString();
    }
    QString epName;
    EpType epType;
    double epIndex;
};
struct DanmuPoolSourceNode : public DanmuPoolNode
{
    DanmuPoolSourceNode(DanmuPoolNode *pNode = nullptr):DanmuPoolNode(DanmuPoolNode::SourecNode,pNode){}
    DanmuPoolSourceNode(const DanmuSource &src, DanmuPoolNode *pNode = nullptr):DanmuPoolNode(DanmuPoolNode::SourecNode,pNode)
    {
        title=src.title;
        idInfo=src.scriptId;
        if (src.isKikoSource()) idInfo = "Kiko";
        srcId=src.id;
        delay=src.delay;
        scriptData=src.scriptData;
        danmuCount = src.count;
        hasTimeline = !src.timelineInfo.isEmpty();

    }
    bool isSameSource(const DanmuSource &src) const { return idInfo==src.scriptId && scriptData==src.scriptData; }
    int srcId, delay;
    bool hasTimeline{false};
    QString scriptData;
};
#endif // NODEINFO_H
