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
class DanmuRender : public QObject
{
    Q_OBJECT
public:
    explicit DanmuRender();
    ~DanmuRender();
    void drawDanmu();
    void moveDanmu(float interval);
    void cleanup(DanmuComment::DanmuType cleanType);
    void cleanup();
    inline void hideDanmu(DanmuComment::DanmuType type,bool hide){hideLayout[type]=hide;}
    QRectF surfaceRect;
    bool dense;
    QSharedPointer<DanmuComment> danmuAt(QPointF point);
    void removeBlocked();
    void drawDanmuTexture(const DanmuObject *danmuObj);
    void refDesc(DanmuDrawInfo *drawInfo);
private:
    DanmuLayout *layout_table[3];
    bool hideLayout[3];
    float danmuOpacity;
    bool bottomSubtitleProtect;
    bool topSubtitleProtect;
    int maxCount;
    int fontSizeTable[3];
    DanmuStyle danmuStyle;
    QThread cacheThread;
    CacheWorker *cacheWorker;
    QList<QList<DanmuDrawInfo *> *> drListPool;
    QList<DanmuDrawInfo *>  *currentDrList;
    void refreshDMRect();
public:
    void setBottomSubtitleProtect(bool bottomOn);
    void setTopSubtitleProtect(bool topOn);
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
    void refCountChanged(QList<DanmuDrawInfo *> *descList);
public slots:
    inline void prepareDanmu(PrepareList *prepareList){emit cacheDanmu(prepareList);}
    void addDanmu(PrepareList *newDanmu);
};

#endif // DANMURENDER_H
