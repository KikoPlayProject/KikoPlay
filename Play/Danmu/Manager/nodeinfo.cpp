#include "nodeinfo.h"

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
