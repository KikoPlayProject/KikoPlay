#include "danmurender.h"
#include "../Layouts/rolllayout.h"
#include "../Layouts/toplayout.h"
#include "../Layouts/bottomlayout.h"
#include <QPair>
#include <QRandomGenerator>
#include <QOffscreenSurface>
#include "globalobjects.h"
#include "Play/Danmu/danmupool.h"
#include "Play/Playlist/playlist.h"
#include "Play/Video/mpvplayer.h"
#include "livedanmulistmodel.h"


QOpenGLContext *danmuTextureContext=nullptr;
QOffscreenSurface *surface=nullptr;

DanmuRender::DanmuRender(QObject *parent) : QObject(parent)
{
    layout_table[DanmuComment::DanmuType::Rolling] = new RollLayout(this);
    layout_table[DanmuComment::DanmuType::Top] = new TopLayout(this);
    layout_table[DanmuComment::DanmuType::Bottom] = new BottomLayout(this);
    hideLayout[DanmuComment::DanmuType::Rolling] = false;
    hideLayout[DanmuComment::DanmuType::Top] = false;
    hideLayout[DanmuComment::DanmuType::Bottom] = false;
    bottomSubtitleProtect = true;
    topSubtitleProtect = false;
    displayAreaRatio = 1.0f;
    dense = 0;
    maxCount = -1;
    danmuOpacity = 1;
    fontSizeTable[0] = 20;
    fontSizeTable[1] = fontSizeTable[0] / 1.5;
    fontSizeTable[2] = fontSizeTable[0] * 1.5;
    danmuStyle.fontSizeTable = fontSizeTable;
    danmuStyle.fontFamily = "Microsoft YaHei";
    danmuStyle.randomSize = false;
    danmuStyle.strokeWidth = 3.5;
	danmuStyle.bold = false;
    danmuStyle.enlargeMerged = true;
    danmuStyle.mergeCountPos = 1;
    danmuStyle.glow = false;
    danmuStyle.glowRadius = 16;
    enableLiveMode = false;
    liveModeOnlyRolling = true;
    liveDanmuListModel = new LiveDanmuListModel(this);
    liveDanmuListModel->setFontFamily(danmuStyle.fontFamily);

    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::resized,this,&DanmuRender::refreshDMRect);

    cacheWorker=new CacheWorker(&danmuStyle);
    cacheWorker->moveToThread(&cacheThread);
    QObject::connect(&cacheThread, &QThread::finished, cacheWorker, &QObject::deleteLater);
    QObject::connect(this,&DanmuRender::cacheDanmu,cacheWorker,&CacheWorker::beginCache);
    QObject::connect(this,&DanmuRender::refCountChanged,cacheWorker,&CacheWorker::changeRefCount);
    QObject::connect(cacheWorker,&CacheWorker::recyleRefList,[this](QVector<DanmuDrawInfo *> *drList){
        drListPool.append(drList);
    });
    QObject::connect(cacheWorker,&CacheWorker::cacheDone,this,&DanmuRender::addDanmu);
    QObject::connect(this,&DanmuRender::danmuStyleChanged,cacheWorker,&CacheWorker::changeDanmuStyle);

    QObject::connect(GlobalObjects::mpvplayer,&MPVPlayer::initContext,[this](){
        QOpenGLContext *sharectx = GlobalObjects::mpvplayer->context();
        danmuTextureContext = new QOpenGLContext();
        danmuTextureContext->setFormat(sharectx->format());
        danmuTextureContext->setShareContext(sharectx);
        danmuTextureContext->create();
        surface = new QOffscreenSurface();
        surface->setFormat(sharectx->format());
        surface->create();
#ifndef TEXTURE_MAIN_THREAD
        danmuTextureContext->moveToThread(&cacheThread);
#endif
    });
    currentDrList=nullptr;

    cacheThread.setObjectName(QStringLiteral("cacheThread"));
    cacheThread.start(QThread::NormalPriority);
}

DanmuRender::~DanmuRender()
{
    delete layout_table[DanmuComment::DanmuType::Rolling];
    delete layout_table[DanmuComment::DanmuType::Top];
    delete layout_table[DanmuComment::DanmuType::Bottom];
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
    liveDanmuListModel->clear();
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
    const int drSize = 64;
    if(!currentDrList)
    {
        if(drListPool.isEmpty())
        {
            currentDrList=new QVector<DanmuDrawInfo *>();
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
        QVector<DanmuDrawInfo *> *tmp=currentDrList;
        currentDrList=nullptr;
        emit refCountChanged(tmp);
    }
}

void DanmuRender::refreshDMRect()
{
    QSize surfaceSize(GlobalObjects::mpvplayer->size());
    surfaceSize.setWidth(surfaceSize.width()*GlobalObjects::mpvplayer->devicePixelRatioF());
    surfaceSize.setHeight(surfaceSize.height()*GlobalObjects::mpvplayer->devicePixelRatioF());
    this->surfaceRect.setRect(0,0,surfaceSize.width(), surfaceSize.height());
    float dpRatio = displayAreaRatio;
    if(bottomSubtitleProtect)
    {
        dpRatio = qMin(dpRatio, 0.85f);
    }
    if(dpRatio < 1.0f)
    {
        this->surfaceRect.setBottom(surfaceSize.height()*dpRatio);
    }
    if(topSubtitleProtect)
    {
        this->surfaceRect.setTop(surfaceSize.height()*0.10);
    }
}

void DanmuRender::setBottomSubtitleProtect(bool bottomOn)
{
    bottomSubtitleProtect = bottomOn;
    refreshDMRect();
}

void DanmuRender::setTopSubtitleProtect(bool topOn)
{
    topSubtitleProtect = topOn;
    refreshDMRect();
}

void DanmuRender::setDisplayArea(float ratio)
{
    displayAreaRatio = ratio;
    refreshDMRect();
}

void DanmuRender::setFontSize(int pt)
{
    fontSizeTable[0] = pt;
    fontSizeTable[1] = fontSizeTable[0]/1.5;
    fontSizeTable[2] = fontSizeTable[0]*1.5;
}

void DanmuRender::setBold(bool bold)
{
    danmuStyle.bold = bold;
    emit danmuStyleChanged();
}

void DanmuRender::setGlow(bool on)
{
    danmuStyle.glow = on;
}

void DanmuRender::setOpacity(float opacity)
{
    danmuOpacity = qBound(0.f,opacity,1.f);
    liveDanmuListModel->setAlpha(danmuOpacity);
}

void DanmuRender::setFontFamily(const QString &family)
{
    danmuStyle.fontFamily = family;
    liveDanmuListModel->setFontFamily(danmuStyle.fontFamily);
    emit danmuStyleChanged();
}

void DanmuRender::setSpeed(float speed)
{
    static_cast<RollLayout *>(layout_table[0])->setSpeed(speed);
}

void DanmuRender::setStrokeWidth(float width)
{
    danmuStyle.strokeWidth = width;
}

void DanmuRender::setRandomSize(bool randomSize)
{
    danmuStyle.randomSize = randomSize;
}

void DanmuRender::setMaxDanmuCount(int count)
{
    maxCount = count;
}

void DanmuRender::setMergeCountPos(int pos)
{
    danmuStyle.mergeCountPos = pos;
    liveDanmuListModel->setMergeCountPos(pos);
}

void DanmuRender::setEnlargeMerged(bool enlarge)
{
    danmuStyle.enlargeMerged = enlarge;
    liveDanmuListModel->setEnlargeMerged(enlarge);
}

void DanmuRender::setLiveMode(bool on, bool onlyRolling)
{
    enableLiveMode = on;
    liveModeOnlyRolling = onlyRolling;
}

void DanmuRender::prepareDanmu(QVector<DrawTask> *prepareList)
{
    if(maxCount != -1)
    {
        if(layout_table[DanmuComment::Rolling]->danmuCount()+
           layout_table[DanmuComment::Top]->danmuCount()+
           layout_table[DanmuComment::Bottom]->danmuCount() > maxCount)
        {
            GlobalObjects::danmuPool->recyclePrepareList(prepareList);
            return;
        }
    }
    if(enableLiveMode)
    {
        QVector<DrawTask> liveList;
        for(const DrawTask &d : qAsConst(*prepareList))
        {
            if(liveModeOnlyRolling && d.comment->type != DanmuComment::Rolling) continue;
            if(!d.isCurrent) continue;
            if(d.comment->type != DanmuComment::UNKNOW && hideLayout[d.comment->type]) continue;
            liveList.append(d);
        }
        liveDanmuListModel->addLiveDanmu(liveList);
        if(liveModeOnlyRolling)
        {
            prepareList->erase(std::remove_if(prepareList->begin(), prepareList->end(), [](const DrawTask &d){
                return d.comment->type == DanmuComment::DanmuType::Rolling;
            }), prepareList->end());
            if(prepareList->isEmpty())
            {
                GlobalObjects::danmuPool->recyclePrepareList(prepareList);
                return;
            }
        }
        else
        {
            GlobalObjects::danmuPool->recyclePrepareList(prepareList);
            return;
        }
    }
    emit cacheDanmu(prepareList);
}

void DanmuRender::addDanmu(QVector<DrawTask> *newDanmu)
{
    if(GlobalObjects::playlist->getCurrentItem() != nullptr)
    {
        for(auto &danmuInfo : *newDanmu)
        {
            if(danmuInfo.isCurrent)
                layout_table[danmuInfo.comment->type]->addDanmu(danmuInfo.comment,danmuInfo.drawInfo);
        }
    }
    GlobalObjects::danmuPool->recyclePrepareList(newDanmu);
}


