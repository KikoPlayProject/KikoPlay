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
    void hideDanmu(DanmuComment::DanmuType type, bool hide);
    bool isHidden(DanmuComment::DanmuType type) { return hideLayout[type]; }
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
    int displayAreaIndex;
    float displayAreaRatio;
    const QVector<float> rangeMapping = { 0.125f, 0.25f, 0.5f, 0.75f, 1.0f };
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
    bool isBottomSubProtect() const { return bottomSubtitleProtect; }
    void setTopSubtitleProtect(bool topOn);
    bool isTopSubProtect() const { return topSubtitleProtect; }
    void setDisplayArea(int index);
    int getDisplayAreaIndex() const { return displayAreaIndex; }
    void setFontSize(int pt);
    int getFontSize() const { return fontSizeTable[0]; }
    void setBold(bool bold);
    bool isBold() const { return danmuStyle.bold; }
    void setGlow(bool on);
    bool isGlow() const { return danmuStyle.glow; }
    void setOpacity(float opacity);
    float getOpacity() const { return danmuOpacity; }
    void setFontFamily(const QString &family);
    QString getFontFamily() const { return danmuStyle.fontFamily; }
    void setSpeed(float speed);
    float getSpeed() const;
    void setStrokeWidth(float width);
    float getStrokeWidth() const { return danmuStyle.strokeWidth; }
    void setRandomSize(bool randomSize);
    bool isRandomSize() const { return danmuStyle.randomSize; }
    void setMaxDanmuCount(int count);
    int getMaxDanmuCount() const { return maxCount; }
    void setMergeCountPos(int pos);
    int getMergeCountPos() const { return danmuStyle.mergeCountPos; }
    void setEnlargeMerged(bool enlarge);
    bool isEnlargeMerged() const { return danmuStyle.enlargeMerged; }
    void setLiveMode(bool on, int onlyRolling = -1);
    bool isLiveModeOn() const { return enableLiveMode; }
    bool  isLiveOnlyRolling() const { return liveModeOnlyRolling; }
    void setDenseLevel(int level);
signals:
    void cacheDanmu(QVector<DrawTask> *newDanmu);
    void danmuStyleChanged();
    void refCountChanged(QVector<DanmuDrawInfo *> *descList);
public slots:
    void prepareDanmu(QVector<DrawTask> *prepareList);
    void addDanmu(QVector<DrawTask> *newDanmu);
};

#endif // DANMURENDER_H
