#ifndef DANMURENDER_H
#define DANMURENDER_H
#include <QtCore>
#include <QtGui>
#include <QList>
#include "common.h"
#include "Layouts/danmulayout.h"
#include "Play/Video/mpvplayer.h"
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
    explicit CacheWorker(QHash<QString,DanmuDrawInfo *> *cache, const DanmuStyle *style);
    DanmuDrawInfo *createDanmuCache(const DanmuComment *comment);
private:
    const int max_cache=300;
    QHash<QString,DanmuDrawInfo *> *danmuCache;
    const DanmuStyle *danmuStyle;
    QFont danmuFont;
    QPen danmuStrokePen;
    void cleanCache();
signals:
    void cacheDone(PrepareList *danmus);
public slots:
    void beginCache(PrepareList *danmus);
    void changeDanmuStyle();
};
class DanmuRender : public QObject
{
    Q_OBJECT
public:
    explicit DanmuRender();
    ~DanmuRender();
    void setSurface(MPVPlayer *surface);
    void drawDanmu(QPainter &painter);
    void moveDanmu(float interval);
    void cleanup(DanmuComment::DanmuType cleanType);
    void cleanup();
    inline void hideDanmu(DanmuComment::DanmuType type,bool hide){hideLayout[type]=hide;}
    QSize surfaceSize;
    bool dense;
    QSharedPointer<DanmuComment> danmuAt(QPointF point);
    void removeBlocked();
private:
    DanmuLayout *layout_table[3];
    bool hideLayout[3];
    QHash<QString,DanmuDrawInfo *> danmuCache;
    float danmuOpacity;
    bool subtitleProtect;
    int maxCount;
    int fontSizeTable[3];
    DanmuStyle danmuStyle;
    QThread cacheThread;
    CacheWorker *cacheWorker;
public:
    void setSubtitleProtect(bool on);
    void setFontSize(int pt);
    void setBold(bool bold);
    void setOpacity(float opacity);
    void setFontFamily(QString &family);
    void setSpeed(float speed);
    void setStrokeWidth(float width);
    void setRandomSize(bool randomSize);
    void setMaxDanmuCount(int count);
signals:
    void cacheDanmu(PrepareList *newDanmu);
    void danmuStyleChanged();
public slots:
    inline void prepareDanmu(PrepareList *prepareList){emit cacheDanmu(prepareList);}
    void addDanmu(PrepareList *newDanmu);
};

#endif // DANMURENDER_H
