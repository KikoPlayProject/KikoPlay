#ifndef ROLLLAYOUT_H
#define ROLLLAYOUT_H
#include "Play/Danmu/Render/danmurender.h"
class RollLayout : public DanmuLayout
{
public:
    RollLayout(DanmuRender *render);

    virtual void addDanmu(QSharedPointer<DanmuComment> danmu,DanmuDrawInfo *drawInfo) override;
    virtual void moveLayout(float step) override;
    virtual void drawLayout() override;
    virtual QSharedPointer<DanmuComment> danmuAt(QPointF point) override;
    inline virtual int danmuCount() override {return rolldanmu.size()+lastcol.size();}
    virtual void cleanup() override;
    virtual ~RollLayout();
    void setSpeed(float speed);
    float getSpeed() const { return base_speed; }
    virtual void removeBlocked() override;

private:
    std::list<DanmuObject *> rolldanmu,lastcol;
    float base_speed;

    inline void moveLayoutList(std::list<DanmuObject *> &list, float step);
    QSharedPointer<DanmuComment> danmuAtList(QPointF point, std::list<DanmuObject *> &list);
    inline bool isCollided(const DanmuObject *d1, const DanmuObject *d2);
};

#endif // ROLLLAYOUT_H
