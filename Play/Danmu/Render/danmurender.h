#ifndef DANMURENDER_H
#define DANMURENDER_H
#include <QtCore>
#include <QtGui>
#include <QList>
#include "../common.h"
#include "../Layouts/danmulayout.h"
#include "cacheworker.h"
class LiveDanmuListModel;
class DanmuRender : public QObject
{
    Q_OBJECT
public:
    explicit DanmuRender(QObject *parent = nullptr);
    ~DanmuRender();
    void drawDanmu();
    void moveDanmu(float interval);
    void cleanup(DanmuComment::DanmuType cleanType);
    void cleanup();
    inline void hideDanmu(DanmuComment::DanmuType type,bool hide){hideLayout[type]=hide;}
    QSharedPointer<DanmuComment> danmuAt(QPointF point);
    void removeBlocked();
    inline void drawDanmuTexture(const DanmuObject *danmuObj){objList<<danmuObj;}
    void refDesc(DanmuDrawInfo *drawInfo);
    LiveDanmuListModel *liveDanmuModel() const { return liveDanmuListModel; }
public:
    QRectF surfaceRect;
    int dense;
private:
    DanmuLayout *layout_table[DanmuComment::DanmuType::UNKNOW];
    bool hideLayout[DanmuComment::DanmuType::UNKNOW];
    float danmuOpacity;
    bool bottomSubtitleProtect;
    bool topSubtitleProtect;
    float displayAreaRatio;
    int maxCount;
    int fontSizeTable[3];
    DanmuStyle danmuStyle;
    QThread cacheThread;
    CacheWorker *cacheWorker;
    bool enableLiveMode;
    bool liveModeOnlyRolling;
    LiveDanmuListModel *liveDanmuListModel;
    QVector<QVector<DanmuDrawInfo *> *> drListPool;
    QVector<DanmuDrawInfo *>  *currentDrList;
    QVector<const DanmuObject *> objList;
    void refreshDMRect();
public:
    void setBottomSubtitleProtect(bool bottomOn);
    void setTopSubtitleProtect(bool topOn);
    void setDisplayArea(float ratio);
    void setFontSize(int pt);
    void setBold(bool bold);
    void setOpacity(float opacity);
    void setFontFamily(const QString &family);
    void setSpeed(float speed);
    void setStrokeWidth(float width);
    void setRandomSize(bool randomSize);
    void setMaxDanmuCount(int count);
    void setMergeCountPos(int pos);
    void setEnlargeMerged(bool enlarge);
    void setLiveMode(bool on, bool onlyRolling);
signals:
    void cacheDanmu(QVector<DrawTask> *newDanmu);
    void danmuStyleChanged();
    void refCountChanged(QVector<DanmuDrawInfo *> *descList);
public slots:
    void prepareDanmu(QVector<DrawTask> *prepareList);
    void addDanmu(QVector<DrawTask> *newDanmu);
};

#endif // DANMURENDER_H
