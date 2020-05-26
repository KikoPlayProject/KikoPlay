#ifndef DANMULAYOUT_H
#define DANMULAYOUT_H
#include <QtCore>
#include <QtGui>
class DanmuRender;
struct DanmuComment;
class DanmuDrawInfo;
class DanmuLayout
{
protected:
    DanmuRender *render;
    const float margin_y=0;
public:
    DanmuLayout(DanmuRender *render)
    {
        this->render=render;
    }
    virtual void addDanmu(QSharedPointer<DanmuComment> danmu,DanmuDrawInfo *drawInfo)=0;
    virtual void moveLayout(float step)=0;
    virtual void drawLayout()=0;
    virtual ~DanmuLayout(){}
    virtual QSharedPointer<DanmuComment> danmuAt(QPointF point)=0;
    virtual void cleanup()=0;
    virtual int danmuCount()=0;
    virtual void removeBlocked()=0;
};

#endif // DANMULAYOUT_H
