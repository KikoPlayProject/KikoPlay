#include "danmurender.h"
#include "Layouts/rolllayout.h"
#include "Layouts/toplayout.h"
#include "Layouts/bottomlayout.h"
#include <QPair>
#include <QRandomGenerator>
#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Playlist/playlist.h"
DanmuRender::DanmuRender()
{
    layout_table[0]=new RollLayout(this);
    layout_table[1]=new TopLayout(this);
    layout_table[2]=new BottomLayout(this);
    hideLayout[0]=hideLayout[1]=hideLayout[2]=false;
    subtitleProtect=true;
    dense=false;
    maxCount=-1;
    danmuOpacity=1;
    danmuStyle.strokeWidth=3.5;
    fontSizeTable[0]=20;
    fontSizeTable[1]=fontSizeTable[0]/1.5;
    fontSizeTable[2]=fontSizeTable[0]*1.5;
    danmuStyle.fontSizeTable=fontSizeTable;
    danmuStyle.fontFamily="Microsoft YaHei";
    danmuStyle.randomSize=false;
	danmuStyle.bold = false;

    cacheWorker=new CacheWorker(&danmuCache,&danmuStyle);
    cacheWorker->moveToThread(&cacheThread);
    QObject::connect(&cacheThread, &QThread::finished, cacheWorker, &QObject::deleteLater);
    QObject::connect(this,&DanmuRender::cacheDanmu,cacheWorker,&CacheWorker::beginCache);
    QObject::connect(cacheWorker,&CacheWorker::cacheDone,this,&DanmuRender::addDanmu);
    QObject::connect(this,&DanmuRender::danmuStyleChanged,cacheWorker,&CacheWorker::changeDanmuStyle);
    cacheThread.setObjectName(QStringLiteral("cacheThread"));
    cacheThread.start(QThread::NormalPriority);
}

DanmuRender::~DanmuRender()
{
    delete layout_table[0];
    delete layout_table[1];
    delete layout_table[2];
    cacheThread.quit();
    cacheThread.wait();
    for(auto iter=danmuCache.cbegin();iter!=danmuCache.cend();++iter)
        delete iter.value();
    DanmuObject::DeleteObjPool();
}

void DanmuRender::setSurface(MPVPlayer *surface)
{
    QObject::connect(surface,&MPVPlayer::resized,[surface,this](){
        this->surfaceSize=surface->size();
        if(subtitleProtect)
        {
            this->surfaceSize.rheight()*=0.85;
        }
    });
}

void DanmuRender::drawDanmu(QPainter &painter)
{
    painter.setOpacity(danmuOpacity);
    if(!hideLayout[0])layout_table[0]->drawLayout(painter);
    if(!hideLayout[1])layout_table[1]->drawLayout(painter);
    if(!hideLayout[2])layout_table[2]->drawLayout(painter);
}

void DanmuRender::moveDanmu(float interval)
{
    layout_table[0]->moveLayout(interval);
    layout_table[1]->moveLayout(interval);
    layout_table[2]->moveLayout(interval);
}

void DanmuRender::cleanup(DanmuComment::DanmuType cleanType)
{
    layout_table[cleanType]->cleanup();
}

void DanmuRender::cleanup()
{
    layout_table[DanmuComment::Rolling]->cleanup();
    layout_table[DanmuComment::Top]->cleanup();
    layout_table[DanmuComment::Bottom]->cleanup();
}

QSharedPointer<DanmuComment> DanmuRender::danmuAt(QPointF point)
{
    auto dm(layout_table[DanmuComment::Rolling]->danmuAt(point));
    if(!dm.isNull())return dm;
    dm=layout_table[DanmuComment::Top]->danmuAt(point);
    if(!dm.isNull())return dm;
    return layout_table[DanmuComment::Bottom]->danmuAt(point);
}

void DanmuRender::removeBlocked()
{
    layout_table[DanmuComment::Rolling]->removeBlocked();
    layout_table[DanmuComment::Top]->removeBlocked();
    layout_table[DanmuComment::Bottom]->removeBlocked();
}

void DanmuRender::setSubtitleProtect(bool on)
{
    subtitleProtect=on;
}

void DanmuRender::setFontSize(int pt)
{
    fontSizeTable[0]=pt;
    fontSizeTable[1]=fontSizeTable[0]/1.5;
    fontSizeTable[2]=fontSizeTable[0]*1.5;
}

void DanmuRender::setBold(bool bold)
{
    danmuStyle.bold=bold;
    emit danmuStyleChanged();
}

void DanmuRender::setOpacity(float opacity)
{
    danmuOpacity=qBound(0.f,opacity,1.f);
}

void DanmuRender::setFontFamily(QString &family)
{
    danmuStyle.fontFamily=family;
    emit danmuStyleChanged();
}

void DanmuRender::setSpeed(float speed)
{
    static_cast<RollLayout *>(layout_table[0])->setSpeed(speed);
}

void DanmuRender::setStrokeWidth(float width)
{
    danmuStyle.strokeWidth=width;
}

void DanmuRender::setRandomSize(bool randomSize)
{
    danmuStyle.randomSize=randomSize;
}

void DanmuRender::setMaxDanmuCount(int count)
{
    maxCount=count;
}

void DanmuRender::addDanmu(PrepareList *newDanmu)
{
    int addCount(0);
    if(GlobalObjects::playlist->getCurrentItem()!=nullptr)
    {
        for(auto &danmuInfo:*newDanmu)
        {
            layout_table[danmuInfo.first->type]->addDanmu(danmuInfo.first,danmuInfo.second);
            addCount++;
            if(maxCount!=-1 && !dense)
            {
                if(layout_table[DanmuComment::Rolling]->danmuCount()+
                   layout_table[DanmuComment::Top]->danmuCount()+
                   layout_table[DanmuComment::Bottom]->danmuCount()+addCount>maxCount)
                    break;
            }
        }
    }
    GlobalObjects::danmuPool->recyclePrepareList(newDanmu);
}

CacheWorker::CacheWorker(QHash<QString, DanmuDrawInfo *> *cache, const DanmuStyle *style):
    danmuCache(cache),danmuStyle(style)
{
    danmuFont.setFamily(danmuStyle->fontFamily);
    danmuStrokePen.setWidthF(danmuStyle->strokeWidth);
    danmuStrokePen.setJoinStyle(Qt::RoundJoin);
    danmuStrokePen.setCapStyle(Qt::RoundCap);
}

DanmuDrawInfo *CacheWorker::createDanmuCache(const DanmuComment *comment)
{
    if(danmuStyle->randomSize)
        danmuFont.setPointSize(QRandomGenerator::global()->
                               bounded(danmuStyle->fontSizeTable[DanmuComment::FontSizeLevel::Small],
                               danmuStyle->fontSizeTable[DanmuComment::FontSizeLevel::Large]));
    else
        danmuFont.setPointSize(danmuStyle->fontSizeTable[comment->fontSizeLevel]);
    QFontMetrics metrics(danmuFont);
    danmuStrokePen.setWidthF(danmuStyle->strokeWidth);
    int strokeWidth=danmuStyle->strokeWidth;
    int left=metrics.leftBearing(comment->text.front());
    if(left<0)left=-left;
    QSize size=metrics.size(0, comment->text)+QSize(strokeWidth*2+left,strokeWidth);
    QImage *img=new QImage(size, QImage::Format_ARGB32_Premultiplied);
    QPainterPath path;
    int py = (size.height() - metrics.height()) / 2 + metrics.ascent();
    if(py < 0) py = -py;
    path.addText(left+strokeWidth,py,danmuFont,comment->text);
    img->fill(Qt::transparent);
    QPainter painter(img);
    painter.setRenderHint(QPainter::Antialiasing);
    int r=comment->color>>16,g=(comment->color>>8)&0xff,b=comment->color&0xff;
    if(strokeWidth>0)
    {
        if(comment->color==0x000000) danmuStrokePen.setColor(Qt::white);
        else danmuStrokePen.setColor(Qt::black);
        painter.strokePath(path,danmuStrokePen);
        painter.drawPath(path);
    }
    QColor color(r,g,b);//(comment->color>>16,(comment->color>>8)&0xff,comment->color&0xff);
    painter.fillPath(path,QBrush(color));
    painter.end();

    DanmuDrawInfo *drawInfo=new DanmuDrawInfo;
    drawInfo->useCount=0;
    drawInfo->height=size.height();
    drawInfo->width=size.width();
    drawInfo->img=img;

    return drawInfo;
}

void CacheWorker::cleanCache()
{
#ifdef QT_DEBUG
    qDebug()<<"clean start, items:"<<danmuCache->size();
    QElapsedTimer timer;
    timer.start();
#endif
    for(auto iter=danmuCache->begin();iter!=danmuCache->end();)
    {
        if(iter.value()->useCount==0)
        {
            delete iter.value();
            iter=danmuCache->erase(iter);
        }
        else
        {
            iter++;
        }
    }
#ifdef QT_DEBUG
    qDebug()<<"clean done:"<<timer.elapsed()<<"ms, left item:"<<danmuCache->size();
#endif
}

void CacheWorker::beginCache(PrepareList *danmus)
{
#ifdef QT_DEBUG
    qDebug()<<"cache start---";
    QElapsedTimer timer;
    timer.start();
    qint64 etime=0;
#endif
    for(QPair<QSharedPointer<DanmuComment>,DanmuDrawInfo*> &dm:*danmus)
    {
         QString hash_str=QString("%1%2%3").arg(dm.first->text).arg(dm.first->color).arg(danmuStyle->fontSizeTable[dm.first->fontSizeLevel]);
         bool dmContains=danmuCache->contains(hash_str);
         DanmuDrawInfo *drawInfo(nullptr);
         if(!dmContains)
         {
             drawInfo=createDanmuCache(dm.first.data());
             drawInfo->useCount++;
             danmuCache->insert(hash_str,drawInfo);
         }
         else
         {
             drawInfo=danmuCache->value(hash_str);
             drawInfo->useCountLock.lock();
             drawInfo->useCount++;
             drawInfo->useCountLock.unlock();
         }
         dm.second=drawInfo;
#ifdef QT_DEBUG
         qDebug()<<"Gen cache:"<<dm.first->text<<" time: "<<dm.first->date;
#endif

    }
	if (danmuCache->size()>max_cache)
		cleanCache();

#ifdef QT_DEBUG
    etime=timer.elapsed();
    qDebug()<<"cache end, time: "<<etime<<"ms";
#endif
    emit cacheDone(danmus);
}

void CacheWorker::changeDanmuStyle()
{
    danmuFont.setFamily(danmuStyle->fontFamily);
    danmuFont.setBold(danmuStyle->bold);
}
