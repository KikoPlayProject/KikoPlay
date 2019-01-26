#ifndef CACHEWORKER_H
#define CACHEWORKER_H
#include <QtCore>
#include "../common.h"
struct DanmuStyle
{
    int *fontSizeTable;
    float strokeWidth;
    bool bold;
    QString fontFamily;
    bool randomSize;
};
class CacheWorker : public QObject
{
    Q_OBJECT
public:
    explicit CacheWorker(const DanmuStyle *style);
private:
    const int max_cache=300;
    QHash<QString,DanmuDrawInfo *> danmuCache;
    const DanmuStyle *danmuStyle;
    QFont danmuFont;
    QPen danmuStrokePen;
    void cleanCache();
    DanmuDrawInfo *createDanmuCache(const DanmuComment *comment);
    void createDanmuTexture(DanmuDrawInfo *drawInfo, const QImage &img);
signals:
    void cacheDone(PrepareList *danmus);
    void recyleRefList(QList<DanmuDrawInfo *> *descList);
public slots:
    void beginCache(PrepareList *danmus);
    void changeRefCount(QList<DanmuDrawInfo *> *descList);
    void changeDanmuStyle();
};
#endif // CACHEWORKER_H
