#ifndef CACHEWORKER_H
#define CACHEWORKER_H
#include <QtCore>
#include "../common.h"
struct DanmuStyle
{
    int *fontSizeTable;
    QString fontFamily;
    float strokeWidth;
    int mergeCountPos;
    bool randomSize;
    bool randomColor;
    bool enlargeMerged;
    bool bold;
    bool glow;
    int glowRadius;
    float devicePixelRatioF;
};
struct CacheKey
{
    QString text;
    int color;
    int fontSize;
    int mergedCount;

    bool operator==(const CacheKey &other) const
    {
        return color == other.color && fontSize == other.fontSize && mergedCount == other.mergedCount && text == other.text;
    }
};
inline size_t qHash(const CacheKey &key, size_t seed = 0)
{
    seed = qHash(key.text, seed);
    seed = qHash(key.color, seed);
    seed = qHash(key.fontSize, seed);
    seed = qHash(key.mergedCount, seed);
    return seed;
}
struct CacheMiddleInfo
{
    CacheKey key;
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
    static QImage createPreviewImage(const DanmuStyle *style, const DanmuComment *comment);
private:
    const int max_cache = 512;
    bool init = false;
    QHash<CacheKey, DanmuDrawInfo *> danmuCache;
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
