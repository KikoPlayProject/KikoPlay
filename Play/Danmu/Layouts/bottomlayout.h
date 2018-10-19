#ifndef BOTTOMLAYOUT_H
#define BOTTOMLAYOUT_H
#include "Play/Danmu/danmurender.h"

class BottomLayout : public DanmuLayout
{
public:
    BottomLayout(DanmuRender *render);

    virtual void addDanmu(QSharedPointer<DanmuComment> danmu,DanmuDrawInfo *drawInfo) override;
    virtual void moveLayout(float step) override;
    virtual void drawLayout() override;
    virtual QSharedPointer<DanmuComment> danmuAt(QPointF point) override;
    inline virtual int danmuCount(){return bottomdanmu.count();}
    virtual void cleanup() override;
    virtual ~BottomLayout();
    virtual void removeBlocked();
private:
    float life_time;
    QLinkedList<DanmuObject *> bottomdanmu;    
};

#endif // BOTTOMLAYOUT_H
