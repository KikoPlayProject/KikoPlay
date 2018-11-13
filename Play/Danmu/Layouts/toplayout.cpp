#include "toplayout.h"

TopLayout::TopLayout(DanmuRender *render):DanmuLayout(render),life_time(5000)
{

}

void TopLayout::addDanmu(QSharedPointer<DanmuComment> danmu, DanmuDrawInfo *drawInfo)
{
    const QRectF rect=render->surfaceRect;
    float currentY=margin_y+rect.top();
    float dm_height=drawInfo->height;
    DanmuObject *dmobj=new DanmuObject;
    dmobj->src=danmu;
    dmobj->drawInfo=drawInfo;
    dmobj->extraData=life_time;
    dmobj->x=(rect.width()-drawInfo->width)/2;
    bool success=false;
    float maxSpace(0.f),dsY(0.f),cY(rect.top());
    QLinkedList<DanmuObject *>::Iterator msPos = topdanmu.begin();
    for(auto iter=topdanmu.begin();iter!=topdanmu.end();++iter)
    {
        if((*iter)->y-currentY-margin_y>=dm_height)
        {         
            dmobj->y=currentY;
            topdanmu.insert(iter,dmobj);
            success=true;
            break;
        }
        //for dense layout-----
        float tmp((*iter)->y-cY);
        if(tmp>maxSpace)
        {
            maxSpace=tmp;
            dsY=cY+tmp/2;
            msPos=iter;
        }
        //--------
        cY=(*iter)->y+margin_y;
        currentY=cY+(*iter)->drawInfo->height;
        if(currentY+dm_height>=rect.bottom())
            break;
    }
    if(!success)
    {
        do
        {
            if(currentY+dm_height<rect.bottom())
            {
                dmobj->y=currentY;
                topdanmu.append(dmobj);
                break;
            }
            if(render->dense)
            {
                dmobj->y=dsY;
                topdanmu.insert(msPos,dmobj);
                break;
            }
#ifdef QT_DEBUG
            qDebug()<<"top lost: "<<danmu->text<<",send time:"<<danmu->date;
#endif
            delete dmobj;
        }while(false);
    }
}

void TopLayout::moveLayout(float step)
{
    for(auto iter=topdanmu.begin();iter!=topdanmu.end();)
    {
        DanmuObject *current=(*iter);
        //float *lifetime=&static_cast<TopExtra *>(current->extra_data)->lifetime;
        //*lifetime-=step;
        current->extraData-=step;
        if(current->extraData>0)
        {
            ++iter;
        }
        else
        {
            delete current;
            iter=topdanmu.erase(iter);
        }
    }
}

void TopLayout::drawLayout()
{
    for(auto current:topdanmu)
    {
        render->drawDanmuTexture(current);
        //painter.drawImage(current->x,current->y,*current->drawInfo->img);
    }
}

QSharedPointer<DanmuComment> TopLayout::danmuAt(QPointF point)
{
    for(auto iter=topdanmu.cbegin();iter!=topdanmu.cend();++iter)
    {
        DanmuObject *curDMObj=*iter;
        if(curDMObj->x<point.x() && curDMObj->x+curDMObj->drawInfo->width>point.x() &&
                curDMObj->y<point.y() && curDMObj->y+curDMObj->drawInfo->height>point.y())
            return curDMObj->src;
    }
    return nullptr;
}

void TopLayout::cleanup()
{
    qDeleteAll(topdanmu);
    topdanmu.clear();
}

void TopLayout::removeBlocked()
{
    for(auto iter=topdanmu.begin();iter!=topdanmu.end();)
    {
        if((*iter)->src->blockBy!=-1)
        {
            delete *iter;
            iter=topdanmu.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

TopLayout::~TopLayout()
{
    qDeleteAll(topdanmu);
}

