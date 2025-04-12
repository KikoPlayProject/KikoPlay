#ifndef TOPLAYOUT_H
#define TOPLAYOUT_H
#include "Play/Danmu/Render/danmurender.h"

class TopLayout : public DanmuLayout
{
public:
    TopLayout(DanmuRender *render);

    virtual void addDanmu(QSharedPointer<DanmuComment> danmu,DanmuDrawInfo *drawInfo) override;
    virtual void moveLayout(float step) override;
    virtual void drawLayout() override;
    virtual QSharedPointer<DanmuComment> danmuAt(QPointF point) override;
    inline virtual int danmuCount() override {return topdanmu.size();}
    virtual void cleanup() override;
    virtual void removeBlocked() override;
    virtual ~TopLayout();
private:
    float life_time;
    std::list<DanmuObject *> topdanmu;
};

#endif // TOPLAYOUT_H
