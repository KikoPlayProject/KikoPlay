#ifndef RESOURCESCRIPT_H
#define RESOURCESCRIPT_H
#include "scriptbase.h"
#include <QVariant>
struct ResourceItem
{
    QString title;
    QString time;
    QString size;
    QString magnet;
    QString url;
    QVariantMap toMap() const {return {{"title", title}, {"time", time}, {"size", size}, {"magnet", magnet}, {"url", url}};}
};
class ResourceScript : public ScriptBase
{
public:
    ResourceScript();
    ScriptState load(const QString &scriptPath);
public:
    bool needGetDetail() const {return hasDetailFunc;}
public:
    ScriptState search(const QString &keyword, int page, int &totalPage, QList<ResourceItem> &results);
    ScriptState getDetail(const ResourceItem &oldItem, ResourceItem &newItem);
private:
    bool hasDetailFunc;
};

#endif // RESOURCESCRIPT_H
