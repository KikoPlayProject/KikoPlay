#ifndef TOPLAYOUT_H
#define TOPLAYOUT_H
#include "Play/Danmu/danmurender.h"

class TopLayout : public DanmuLayout
{
public:
    TopLayout(DanmuRender *render);

    virtual void addDanmu(QSharedPointer<DanmuComment> danmu,DanmuDrawInfo *drawInfo) override;
    virtual void moveLayout(float step) override;
    virtual void drawLayout(QPainter &painter) override;
    virtual QSharedPointer<DanmuComment> danmuAt(QPointF point) override;
    inline virtual int danmuCount(){return topdanmu.count();}
    virtual void cleanup() override;
    virtual void removeBlocked();
    virtual ~TopLayout();
private:
    float life_time;
    QLinkedList<DanmuObject *> topdanmu;
};

#endif // TOPLAYOUT_H
