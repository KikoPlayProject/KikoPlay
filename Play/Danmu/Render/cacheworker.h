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
    int mergeCountPos;
    bool enlargeMerged;
};
struct CacheMiddleInfo
{
    QString hash;
    DanmuComment *comment;
    DanmuDrawInfo *drawInfo;
    QImage *img;
    int texX,texY;
};

class CacheWorker : public QObject
{
    Q_OBJECT
public:
    explicit CacheWorker(const DanmuStyle *style);
private:
    const int max_cache=512;
    bool init = false;
    QHash<QString,DanmuDrawInfo *> danmuCache;
    QHash<GLuint,int> textureRef;
    const DanmuStyle *danmuStyle;
    QFont danmuFont;
    QPen danmuStrokePen;
    void cleanCache();
    void createImage(CacheMiddleInfo &midInfo);
    void createTexture(QVector<CacheMiddleInfo> &midInfo);
signals:
    void cacheDone(QVector<DrawTask> *danmus);
    void recyleRefList(QVector<DanmuDrawInfo *> *descList);
public slots:
    void beginCache(QVector<DrawTask> *danmus);
    void changeRefCount(QVector<DanmuDrawInfo *> *descList);
    void changeDanmuStyle();
};
#endif // CACHEWORKER_H
