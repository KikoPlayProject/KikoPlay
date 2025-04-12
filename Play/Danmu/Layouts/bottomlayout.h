#ifndef BOTTOMLAYOUT_H
#define BOTTOMLAYOUT_H
#include "Play/Danmu/Render/danmurender.h"

class BottomLayout : public DanmuLayout
{
public:
    BottomLayout(DanmuRender *render);

    virtual void addDanmu(QSharedPointer<DanmuComment> danmu,DanmuDrawInfo *drawInfo) override;
    virtual void moveLayout(float step) override;
    virtual void drawLayout() override;
    virtual QSharedPointer<DanmuComment> danmuAt(QPointF point) override;
    inline virtual int danmuCount() override {return bottomdanmu.size();}
    virtual void cleanup() override;
    virtual ~BottomLayout();
    virtual void removeBlocked() override;
private:
    float life_time;
    std::list<DanmuObject *> bottomdanmu;
};

#endif // BOTTOMLAYOUT_H
