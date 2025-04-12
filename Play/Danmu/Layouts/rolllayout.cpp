#include "rolllayout.h"

RollLayout::RollLayout(DanmuRender *render):DanmuLayout(render),base_speed(200)
{

}

void RollLayout::addDanmu(QSharedPointer<DanmuComment> danmu, DanmuDrawInfo *drawInfo)
{
    const QRectF rect=render->surfaceRect;

    float speed=(drawInfo->width/5+base_speed)/1000;
    float currentY=margin_y+rect.top();
    float dm_height=drawInfo->height;
    DanmuObject *dmobj=new DanmuObject();
    dmobj->src=danmu;
    dmobj->drawInfo=drawInfo;
    dmobj->extraData=speed;
    dmobj->x=rect.width();
    bool success=false;
    float maxChaseSpace(rect.width()/2),maxSpace(0.f),dsY1(0.f),dsY2(0.f),cY(rect.top());
    std::list<DanmuObject *>::iterator msPos1 = lastcol.end(), msPos2 = lastcol.begin();
    for(auto iter=lastcol.begin();iter!=lastcol.end();++iter)
    {
        float cur_height=(*iter)->drawInfo->height;
        if((*iter)->y-currentY-margin_y>=dm_height)
        {
            dmobj->y=currentY;
            lastcol.insert(iter,dmobj);
            success=true;
            break;
        }
        if(!isCollided((*iter),dmobj))
        {
            dmobj->y=currentY;
            rolldanmu.push_back(*iter);
            *iter=dmobj;
            success=true;
            break;
        }
        //for dense layout-----
        //Although overlays occur, they do not occur until some time later
        float chaseSpace(dmobj->x-(*iter)->x-(*iter)->drawInfo->width);
        if(chaseSpace>maxChaseSpace)
        {
            maxChaseSpace=chaseSpace;//(*iter)->x;
            msPos1=iter;
            dsY1=currentY;
        }
        //Insert between two adjacent danmu, find the largest spacing
        float tmp((*iter)->y-cY);
        if(tmp>maxSpace)
        {
            maxSpace=tmp;
            dsY2=cY+tmp/2;
            msPos2=iter;
        }
        //--------
        cY=(*iter)->y+margin_y;
        currentY=cY+cur_height;
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
                lastcol.push_back(dmobj);
                break;
            }
            if(render->dense>0)
            {
                if (msPos1 != lastcol.end())
                {
                    dmobj->y=dsY1;
                    rolldanmu.push_back(*msPos1);
                    *msPos1=dmobj;
                    break;
                }
                else
                {
                    if((render->dense==1 && maxSpace>=dm_height) || render->dense==2)
                    {
                        dmobj->y=dsY2;
                        lastcol.insert(msPos2,dmobj);
                        break;
                    }
                }   
            }
#ifdef QT_DEBUG
            qDebug()<<"roll lost: "<<danmu->text<<",send time:"<<danmu->date;
#endif
            delete dmobj;
        }while (false);
    }
}

void RollLayout::moveLayout(float step)
{
    moveLayoutList(rolldanmu,step);
    moveLayoutList(lastcol,step);
}

void RollLayout::drawLayout()
{
    for(auto current:rolldanmu)
    {
        //painter.drawImage(current->x,current->y,*current->drawInfo->img);
        render->drawDanmuTexture(current);
    }
    for(auto current:lastcol)
    {
        render->drawDanmuTexture(current);
        //painter.drawImage(current->x,current->y,*current->drawInfo->img);
    }
}

RollLayout::~RollLayout()
{
    qDeleteAll(rolldanmu);
    qDeleteAll(lastcol);
}

QSharedPointer<DanmuComment> RollLayout::danmuAt(QPointF point)
{
    auto ret=danmuAtList(point,rolldanmu);
    if(!ret.isNull())
        return ret;
    ret=danmuAtList(point,lastcol);
    return ret;
}

void RollLayout::cleanup()
{
    qDeleteAll(lastcol);
    qDeleteAll(rolldanmu);
    lastcol.clear();
    rolldanmu.clear();
}

void RollLayout::setSpeed(float speed)
{
    base_speed=speed;
    for(auto iter=rolldanmu.cbegin();iter!=rolldanmu.cend();++iter)
    {
        (*iter)->extraData=((*iter)->drawInfo->width/5+base_speed)/1000;
    }
    for(auto iter=lastcol.cbegin();iter!=lastcol.cend();++iter)
    {
        (*iter)->extraData=((*iter)->drawInfo->width/5+base_speed)/1000;
    }
}

void RollLayout::removeBlocked()
{
    for(auto iter=rolldanmu.begin();iter!=rolldanmu.end();)
    {
        if((*iter)->src->blockBy!=-1)
        {
            delete *iter;
            iter=rolldanmu.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    for(auto iter=lastcol.begin();iter!=lastcol.end();)
    {
        if((*iter)->src->blockBy!=-1)
        {
            delete *iter;
            iter=lastcol.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
}

void RollLayout::moveLayoutList(std::list<DanmuObject *> &list, float step)
{
    for(auto iter=list.begin();iter!=list.end();)
    {
        DanmuObject *current=(*iter);
        DanmuDrawInfo *current_drawInfo=current->drawInfo;
        current->x-=step*current->extraData;
        if(current->x+current_drawInfo->width>0)
        {
            ++iter;
        }
        else
        {
            delete current;
            iter=list.erase(iter);
        }
    }
}

QSharedPointer<DanmuComment> RollLayout::danmuAtList(QPointF point, std::list<DanmuObject *> &list)
{
    for(auto iter=list.cbegin();iter!=list.cend();++iter)
    {
        DanmuObject *curDMObj=*iter;
        if(curDMObj->x<point.x() && curDMObj->x+curDMObj->drawInfo->width>point.x() &&
                curDMObj->y<point.y() && curDMObj->y+curDMObj->drawInfo->height>point.y())
            return curDMObj->src;
    }
    return nullptr;
}

bool RollLayout::isCollided(const DanmuObject *d1, const DanmuObject *d2)
{
    float s1=d1->extraData,s2=d2->extraData;
    float x1w=d1->x+d1->drawInfo->width,x2=d2->x;
    if(x1w>x2)return true;
    if(s2<=s1)return false;
    float t1=x1w/s1,t2=(x2-x1w)/(s2-s1);
    return t2<t1;
}
