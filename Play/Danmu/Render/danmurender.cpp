#include "danmurender.h"
#include "../Layouts/rolllayout.h"
#include "../Layouts/toplayout.h"
#include "../Layouts/bottomlayout.h"
#include <QPair>
#include <QRandomGenerator>
#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Playlist/playlist.h"


QOpenGLContext *danmuTextureContext=nullptr;
QSurface *surface=nullptr;

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
    danmuStyle.enlargeMerged=true;
    danmuStyle.mergeCountPos=1;
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
#ifndef TEXTURE_MAIN_THREAD
        danmuTextureContext->moveToThread(&cacheThread);
#endif
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
    if(!hideLayout[DanmuComment::Rolling])layout_table[DanmuComment::Rolling]->drawLayout();
    if(!hideLayout[DanmuComment::Top])layout_table[DanmuComment::Top]->drawLayout();
    if(!hideLayout[DanmuComment::Bottom])layout_table[DanmuComment::Bottom]->drawLayout();
    GlobalObjects::mpvplayer->drawTexture(objList,danmuOpacity);
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

void DanmuRender::setMergeCountPos(int pos)
{
    danmuStyle.mergeCountPos=pos;
}

void DanmuRender::setEnlargeMerged(bool enlarge)
{
    danmuStyle.enlargeMerged=enlarge;
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


