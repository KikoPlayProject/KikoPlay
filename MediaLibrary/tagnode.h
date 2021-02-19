#ifndef TAGNODE_H
#define TAGNODE_H
#include <QList>

struct TagNode
{
    enum TagType
    {
        TAG_SCRIPT, TAG_TIME, TAG_FILE, TAG_CUSTOM, TAG_ROOT_CATE, TAG_ROOT
    };

    TagNode(const QString &title, TagNode *p=nullptr, int count=0, TagType type = TAG_CUSTOM);
    ~TagNode();
    void setAnimeCount(int newCount);
    void removeSubNode(int row);

    QString tagTitle, tagFilter;
    int animeCount;
    TagType tagType;
    TagNode *parent;
    QList<TagNode *> *subNodes;
    int checkStatus;
};

#endif // TAGNODE_H
