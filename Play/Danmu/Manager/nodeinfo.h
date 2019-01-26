#ifndef NODEINFO_H
#define NODEINFO_H
#include <QtCore>
#include "../common.h"
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
struct DanmuPoolSourceNode : public DanmuPoolNode
{
    DanmuPoolSourceNode(DanmuPoolNode *pNode = nullptr):DanmuPoolNode(DanmuPoolNode::SourecNode,pNode){}
    int srcId;
    int delay;
    QString timeline;
    DanmuSourceInfo toSourceInfo();
    void setTimeline(const DanmuSourceInfo &srcInfo);
};
#endif // NODEINFO_H
