#include "bottomlayout.h"

BottomLayout::BottomLayout(DanmuRender *render):DanmuLayout(render),life_time(5000)
{

}

void BottomLayout::addDanmu(QSharedPointer<DanmuComment> danmu, DanmuDrawInfo *drawInfo)
{
    const QSize size=render->surfaceSize;
    float currentY=size.height()-margin_y;
    float dm_height=drawInfo->height;
    DanmuObject *dmobj=new DanmuObject;
    dmobj->src=danmu;
    dmobj->drawInfo=drawInfo;
    dmobj->extraData=life_time;
    dmobj->x=(size.width()-drawInfo->width)/2;
    bool success=false;
    float maxSpace(0.f),dsY(0.f),cY(size.height());
    QLinkedList<DanmuObject *>::Iterator msPos;
    for(auto iter=bottomdanmu.begin();iter!=bottomdanmu.end();iter++)
    {
        if(currentY-(*iter)->y-(*iter)->drawInfo->height-margin_y>=dm_height)
        {
            dmobj->y=currentY;
            bottomdanmu.insert(iter,dmobj);
            success=true;
            break;
        }
        //for dense layout-----
        float tmp(cY-(*iter)->y);
        if(tmp>maxSpace)
        {
            maxSpace=tmp;
            dsY=cY-tmp/2;
            msPos=iter;
        }
        //--------
        cY=(*iter)->y-margin_y;
        currentY=cY-dm_height;
        if(currentY<=0)
            break;
    }
    if(!success && currentY>0)
    {
        dmobj->y=currentY;
        bottomdanmu.append(dmobj);
        success=true;
    }
    if(!success && render->dense)
    {
        dmobj->y=dsY;
        bottomdanmu.insert(msPos,dmobj);
        success=true;
    }
    if(!success)
    {
#ifdef QT_DEBUG
        qDebug()<<"bottom lost: "<<danmu->text<<",send time:"<<danmu->date;
#endif
        delete dmobj;
    }
}

void BottomLayout::moveLayout(float step)
{
    for(auto iter=bottomdanmu.begin();iter!=bottomdanmu.end();)
    {
        DanmuObject *current=(*iter);
        //float *lifetime=&static_cast<BottomExtra *>(current->extra_data)->lifetime;
        current->extraData-=step;
        //*lifetime-=step;
        if(current->extraData>0)
        {
            iter++;
        }
        else
        {
            delete current;
            iter=bottomdanmu.erase(iter);
        }
    }
}

void BottomLayout::drawLayout(QPainter &painter)
{
    for(auto current:bottomdanmu)
    {
        painter.drawImage(current->x,current->y,*current->drawInfo->img);
    }
}

QSharedPointer<DanmuComment> BottomLayout::danmuAt(QPointF point)
{
    for(auto iter=bottomdanmu.cbegin();iter!=bottomdanmu.cend();iter++)
    {
        DanmuObject *curDMObj=*iter;
        if(curDMObj->x<point.x() && curDMObj->x+curDMObj->drawInfo->width>point.x() &&
                curDMObj->y<point.y() && curDMObj->y+curDMObj->drawInfo->height>point.y())
            return curDMObj->src;
    }
    return nullptr;
}

void BottomLayout::cleanup()
{
    qDeleteAll(bottomdanmu);
    bottomdanmu.clear();
}

BottomLayout::~BottomLayout()
{
    qDeleteAll(bottomdanmu);
}

void BottomLayout::removeBlocked()
{
    for(auto iter=bottomdanmu.begin();iter!=bottomdanmu.end();)
    {
        if((*iter)->src->blockBy!=-1)
        {
            delete *iter;
            iter=bottomdanmu.erase(iter);
        }
        else
        {
            iter++;
        }
    }
}


