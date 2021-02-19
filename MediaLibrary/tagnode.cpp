#include "tagnode.h"


TagNode::TagNode(const QString &title, TagNode *p, int count, TagNode::TagType type):tagTitle(title),tagFilter(title),animeCount(count),tagType(type),
    parent(p),subNodes(nullptr), checkStatus(Qt::Unchecked)
{
    if(p)
    {
        if(!p->subNodes) p->subNodes=new QList<TagNode *>();
        p->subNodes->append(this);
        if(count>0) p->setAnimeCount(p->animeCount + count);
    }
}

TagNode::~TagNode()
{
    if(subNodes)
    {
        qDeleteAll(*subNodes);
        delete subNodes;
    }
}

void TagNode::setAnimeCount(int newCount)
{
    int delta = newCount - this->animeCount;
    this->animeCount = newCount;
    TagNode *p = parent;
    while(p)
    {
        p->animeCount += delta;
        p = p->parent;
    }
}

void TagNode::removeSubNode(int row)
{
    if(subNodes && row >=0 && row < subNodes->size())
    {
        TagNode *subNode = subNodes->at(row);
        subNodes->removeAt(row);
        delete subNode;
    }
}

