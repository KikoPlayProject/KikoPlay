#include "danmurender.h"
#include "Layouts/rolllayout.h"
#include "Layouts/toplayout.h"
#include "Layouts/bottomlayout.h"
#include <QPair>
#include <QRandomGenerator>
#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Playlist/playlist.h"
namespace
{
    QOpenGLContext *danmuTextureContext=nullptr;
    QSurface *surface=nullptr;
}
DanmuRender::DanmuRender()
{
    layout_table[0]=new RollLayout(this);
    layout_table[1]=new TopLayout(this);
    layout_table[2]=new BottomLayout(this);
    hideLayout[0]=hideLayout[1]=hideLayout[2]=false;
    bottomSubtitleProtect=true;
    topSubtitleProtect=false;
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
    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::resized,this,&DanmuRender::refreshDMRect);

    cacheWorker=new CacheWorker(&danmuStyle);
    cacheWorker->moveToThread(&cacheThread);
    QObject::connect(&cacheThread, &QThread::finished, cacheWorker, &QObject::deleteLater);
    QObject::connect(this,&DanmuRender::cacheDanmu,cacheWorker,&CacheWorker::beginCache);
    QObject::connect(this,&DanmuRender::refCountChanged,cacheWorker,&CacheWorker::changeRefCount);
    QObject::connect(cacheWorker,&CacheWorker::recyleRefList,[this](QList<DanmuDrawInfo *> *drList){
        drListPool.append(drList);
    });
    QObject::connect(cacheWorker,&CacheWorker::cacheDone,this,&DanmuRender::addDanmu);
    QObject::connect(this,&DanmuRender::danmuStyleChanged,cacheWorker,&CacheWorker::changeDanmuStyle);
    cacheThread.setObjectName(QStringLiteral("cacheThread"));
    cacheThread.start(QThread::NormalPriority);

    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::initContext,[this](){
        QOpenGLContext *sharectx = GlobalObjects::mpvplayer->context();
        surface = sharectx->surface();

        danmuTextureContext = new QOpenGLContext();
        danmuTextureContext->setFormat(sharectx->format());
        danmuTextureContext->setShareContext(sharectx);
        danmuTextureContext->create();
        danmuTextureContext->moveToThread(&cacheThread);
    });
    currentDrList=nullptr;
}

DanmuRender::~DanmuRender()
{
    delete layout_table[0];
    delete layout_table[1];
    delete layout_table[2];
    cacheThread.quit();
    cacheThread.wait();
    qDeleteAll(drListPool);
    DanmuObject::DeleteObjPool();
}

void DanmuRender::drawDanmu()
{
    //painter.setOpacity(danmuOpacity);
    if(!hideLayout[DanmuComment::Rolling])layout_table[DanmuComment::Rolling]->drawLayout();
    if(!hideLayout[DanmuComment::Top])layout_table[DanmuComment::Top]->drawLayout();
    if(!hideLayout[DanmuComment::Bottom])layout_table[DanmuComment::Bottom]->drawLayout();
}

void DanmuRender::moveDanmu(float interval)
{
    layout_table[DanmuComment::Rolling]->moveLayout(interval);
    layout_table[DanmuComment::Top]->moveLayout(interval);
    layout_table[DanmuComment::Bottom]->moveLayout(interval);
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
    auto dm(layout_table[DanmuComment::Top]->danmuAt(point));
    if(!dm.isNull())return dm;
    dm=layout_table[DanmuComment::Rolling]->danmuAt(point);
    if(!dm.isNull())return dm;
    return layout_table[DanmuComment::Bottom]->danmuAt(point);
}

void DanmuRender::removeBlocked()
{
    layout_table[DanmuComment::Rolling]->removeBlocked();
    layout_table[DanmuComment::Top]->removeBlocked();
    layout_table[DanmuComment::Bottom]->removeBlocked();
}

void DanmuRender::drawDanmuTexture(const DanmuObject *danmuObj)
{
    static QRectF rect;
    rect.setRect(danmuObj->x,danmuObj->y,danmuObj->drawInfo->width,danmuObj->drawInfo->height);
    GlobalObjects::mpvplayer->drawTexture(danmuObj->drawInfo->texture,danmuOpacity,rect);
}

void DanmuRender::refDesc(DanmuDrawInfo *drawInfo)
{
    const int drSize=64;
    if(!currentDrList)
    {
        if(drListPool.isEmpty())
        {
            currentDrList=new QList<DanmuDrawInfo *>();
            currentDrList->reserve(drSize);
        }
        else
        {
            currentDrList=drListPool.takeFirst();
        }
    }
    currentDrList->append(drawInfo);
    if(currentDrList->size()>=drSize)
    {
        QList<DanmuDrawInfo *> *tmp=currentDrList;
        currentDrList=nullptr;
        emit refCountChanged(tmp);
    }
}

void DanmuRender::refreshDMRect()
{
    const QSize surfaceSize(GlobalObjects::mpvplayer->size());
    this->surfaceRect.setRect(0,0,surfaceSize.width(),surfaceSize.height());
    if(bottomSubtitleProtect)
    {
        this->surfaceRect.setBottom(surfaceSize.height()*0.85);
    }
    if(topSubtitleProtect)
    {
        this->surfaceRect.setTop(surfaceSize.height()*0.10);
    }
}

void DanmuRender::setBottomSubtitleProtect(bool bottomOn)
{
    bottomSubtitleProtect=bottomOn;
    refreshDMRect();
}

void DanmuRender::setTopSubtitleProtect(bool topOn)
{
    topSubtitleProtect=topOn;
    refreshDMRect();

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

void DanmuRender::setFontFamily(const QString &family)
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

void DanmuRender::prepareDanmu(PrepareList *prepareList)
{
    if(maxCount!=-1)
    {
        if(layout_table[DanmuComment::Rolling]->danmuCount()+
           layout_table[DanmuComment::Top]->danmuCount()+
           layout_table[DanmuComment::Bottom]->danmuCount()>maxCount)
        {
            GlobalObjects::danmuPool->recyclePrepareList(prepareList);
            return;
        }
    }
    emit cacheDanmu(prepareList);
}

void DanmuRender::addDanmu(PrepareList *newDanmu)
{
    if(GlobalObjects::playlist->getCurrentItem()!=nullptr)
    {
        for(auto &danmuInfo:*newDanmu)
        {
            layout_table[danmuInfo.first->type]->addDanmu(danmuInfo.first,danmuInfo.second);
        }
    }
    GlobalObjects::danmuPool->recyclePrepareList(newDanmu);
}

CacheWorker::CacheWorker(const DanmuStyle *style):danmuStyle(style)
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
    int left=qAbs(metrics.leftBearing(comment->text.front()));

    QSize size=metrics.size(0, comment->text)+QSize(strokeWidth*2+left,strokeWidth);
    QImage img(size, QImage::Format_ARGB32);

    DanmuDrawInfo *drawInfo=new DanmuDrawInfo;
    drawInfo->useCount=0;
    drawInfo->height=size.height();
    drawInfo->width=size.width();
    //drawInfo->img=img;

    QPainterPath path;
    QStringList multilines(comment->text.split('\n'));
    int py = qAbs((size.height() - metrics.height()*multilines.size()) / 2 + metrics.ascent());
    int i=0;
    for(const QString &line:multilines)
    {
        path.addText(left+strokeWidth,py+i*metrics.height(),danmuFont,line);
        ++i;
    }
    img.fill(Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    int r=comment->color>>16,g=(comment->color>>8)&0xff,b=comment->color&0xff;
    if(strokeWidth>0)
    {
        danmuStrokePen.setColor(comment->color==0x000000?Qt::white:Qt::black);
        painter.strokePath(path,danmuStrokePen);
        painter.drawPath(path);
    }
    painter.fillPath(path,QBrush(QColor(r,g,b)));
    painter.end();

    createDanmuTexture(drawInfo,img);
    return drawInfo;
}

void CacheWorker::cleanCache()
{
#ifdef QT_DEBUG
    qDebug()<<"clean start, items:"<<danmuCache.size();
    QElapsedTimer timer;
    timer.start();
#endif
    danmuTextureContext->makeCurrent(surface);
    QOpenGLFunctions *glFuns=danmuTextureContext->functions();
    for(auto iter=danmuCache.begin();iter!=danmuCache.end();)
    {
		Q_ASSERT(iter.value()->useCount >= 0);
        if(iter.value()->useCount==0)
        {
            glFuns->glDeleteTextures(1,&iter.value()->texture);
            delete iter.value();
            iter=danmuCache.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    danmuTextureContext->doneCurrent();
#ifdef QT_DEBUG
    qDebug()<<"clean done:"<<timer.elapsed()<<"ms, left item:"<<danmuCache.size();
#endif
}

void CacheWorker::createDanmuTexture(DanmuDrawInfo *drawInfo, const QImage &img)
{

    danmuTextureContext->makeCurrent(surface);
    QOpenGLFunctions *glFuns=danmuTextureContext->functions();
    glFuns->glGenTextures(1, &drawInfo->texture);
    glFuns->glBindTexture(GL_TEXTURE_2D, drawInfo->texture);
    glFuns->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glFuns->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, drawInfo->width, drawInfo->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.bits());
    glFuns->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glFuns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFuns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFuns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glFuns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    danmuTextureContext->doneCurrent();
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
         QString hash_str(QString("%1%2%3").arg(dm.first->text).arg(dm.first->color).arg(danmuStyle->fontSizeTable[dm.first->fontSizeLevel]));
         DanmuDrawInfo *drawInfo(danmuCache.value(hash_str,nullptr));
         if(!drawInfo)
         {
             drawInfo=createDanmuCache(dm.first.data());
             danmuCache.insert(hash_str,drawInfo);
         }
         drawInfo->useCount++;
         dm.second=drawInfo;
#ifdef QT_DEBUG
         qDebug()<<"Gen cache:"<<dm.first->text<<" time: "<<dm.first->date;
#endif

    }
    /*std::sort(danmus->first(),danmus->last(),
              [](const QPair<QSharedPointer<DanmuComment>,DanmuDrawInfo*> &item1,
                 const QPair<QSharedPointer<DanmuComment>,DanmuDrawInfo*> &item2)
    {
        static int orderTable[]={0,2,1};
        return orderTable[item1.first->type]>orderTable[item2.first->type];
    });*/
#ifdef QT_DEBUG
    etime=timer.elapsed();
    qDebug()<<"cache end, time: "<<etime<<"ms";
#endif
    emit cacheDone(danmus);
}

void CacheWorker::changeRefCount(QList<DanmuDrawInfo *> *descList)
{
    for(DanmuDrawInfo *drawInfo:*descList)
        drawInfo->useCount--;
    descList->clear();
    emit recyleRefList(descList);
    if (danmuCache.size()>max_cache)
        cleanCache();
}

void CacheWorker::changeDanmuStyle()
{
    danmuFont.setFamily(danmuStyle->fontFamily);
    danmuFont.setBold(danmuStyle->bold);
}

