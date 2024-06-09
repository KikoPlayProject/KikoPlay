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

#define SETTING_KEY_BOTTOM_SUB_PROTECT "Play/BottomSubProtect"
#define SETTING_KEY_TOP_SUB_PROTECT "Play/TopSubProtect"
#define SETTING_KEY_DANMU_SPEED "Play/DanmuSpeed"
#define SETTING_KEY_MAX_COUNT "Play/MaxCount"
#define SETTING_KEY_DENSE_LEVEL "Play/Dense"
#define SETTING_KEY_DISPLAY_AREA "Play/DisplayArea"
#define SETTING_KEY_DANMU_ALPHA "Play/DanmuAlpha"
#define SETTING_KEY_DANMU_STROKE "Play/DanmuStroke"
#define SETTING_KEY_DANMU_FONT_SIZE "Play/DanmuFontSize"
#define SETTING_KEY_DANMU_GLOW "Play/DanmuGlow"
#define SETTING_KEY_DANMU_BOLD "Play/DanmuBold"
#define SETTING_KEY_DANMU_RANDOM_SIZE "Play/RandomSize"
#define SETTING_KEY_DANMU_FONT "Play/DanmuFont"
#define SETTING_KEY_ENLARGE_MERGED "Play/EnlargeMerged"
#define SETTING_KEY_MERGE_COUNT_POS "Play/MergeCountTip"
#define SETTING_KEY_ENABLE_LIVE_MODE "Play/EnableLiveMode"
#define SETTING_KEY_LIVE_MODE_ONLY_ROLLING "Play/LiveModeOnlyRolling"


QOpenGLContext *danmuTextureContext=nullptr;
QOffscreenSurface *surface=nullptr;

namespace
{

static const char *SETTING_KEY_HIDE_TYPE[DanmuComment::DanmuType::UNKNOW] = {
    "Play/HideRolling",
    "Play/HideTop",
    "Play/HideBottom"
};

}

DanmuRender::DanmuRender(QObject *parent) : QObject(parent)
{
    layout_table[DanmuComment::DanmuType::Rolling] = new RollLayout(this);
    layout_table[DanmuComment::DanmuType::Top] = new TopLayout(this);
    layout_table[DanmuComment::DanmuType::Bottom] = new BottomLayout(this);

    for (int i = 0; i < DanmuComment::DanmuType::UNKNOW; ++i)
    {
        hideLayout[i] = GlobalObjects::appSetting->value(SETTING_KEY_HIDE_TYPE[i], false).toBool();
    }
    bottomSubtitleProtect = GlobalObjects::appSetting->value(SETTING_KEY_BOTTOM_SUB_PROTECT, true).toBool();
    topSubtitleProtect = GlobalObjects::appSetting->value(SETTING_KEY_TOP_SUB_PROTECT, false).toBool();

    const int rollingSpeed = GlobalObjects::appSetting->value(SETTING_KEY_DANMU_SPEED, 200).toInt();
    static_cast<RollLayout *>(layout_table[DanmuComment::DanmuType::Rolling])->setSpeed(rollingSpeed);

    displayAreaIndex = GlobalObjects::appSetting->value(SETTING_KEY_DISPLAY_AREA, 4).toInt();
    displayAreaRatio = rangeMapping[displayAreaIndex];
    dense = GlobalObjects::appSetting->value(SETTING_KEY_DENSE_LEVEL, 1).toInt();
    maxCount = GlobalObjects::appSetting->value(SETTING_KEY_MAX_COUNT, 100).toInt();
    danmuOpacity = GlobalObjects::appSetting->value(SETTING_KEY_DANMU_ALPHA, 60).toInt() / 100.f;
    fontSizeTable[0] = GlobalObjects::appSetting->value(SETTING_KEY_DANMU_FONT_SIZE, 20).toInt();
    fontSizeTable[1] = fontSizeTable[0] / 1.5;
    fontSizeTable[2] = fontSizeTable[0] * 1.5;
    danmuStyle.fontSizeTable = fontSizeTable;
    danmuStyle.fontFamily = GlobalObjects::appSetting->value(SETTING_KEY_DANMU_FONT, "Microsoft Yahei").toString();
    danmuStyle.randomSize = GlobalObjects::appSetting->value(SETTING_KEY_DANMU_RANDOM_SIZE, false).toBool();
    danmuStyle.strokeWidth = GlobalObjects::appSetting->value(SETTING_KEY_DANMU_STROKE, 35).toInt() / 10.f;
    danmuStyle.bold = GlobalObjects::appSetting->value(SETTING_KEY_DANMU_BOLD, false).toBool();
    danmuStyle.enlargeMerged = GlobalObjects::appSetting->value(SETTING_KEY_ENLARGE_MERGED, true).toBool();;
    danmuStyle.mergeCountPos = GlobalObjects::appSetting->value(SETTING_KEY_MERGE_COUNT_POS, 1).toInt();
    danmuStyle.glow = GlobalObjects::appSetting->value(SETTING_KEY_DANMU_GLOW, false).toBool();
    danmuStyle.glowRadius = 16;

    enableLiveMode = GlobalObjects::appSetting->value(SETTING_KEY_ENABLE_LIVE_MODE, false).toBool();;
    liveModeOnlyRolling = GlobalObjects::appSetting->value(SETTING_KEY_LIVE_MODE_ONLY_ROLLING, true).toBool();
    liveDanmuListModel = new LiveDanmuListModel(this);
    liveDanmuListModel->setFontFamily(danmuStyle.fontFamily);
    liveDanmuListModel->setAlpha(danmuOpacity);

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

void DanmuRender::hideDanmu(DanmuComment::DanmuType type, bool hide)
{
    hideLayout[type] = hide;
    GlobalObjects::appSetting->setValue(SETTING_KEY_HIDE_TYPE[type], hide);
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
    GlobalObjects::appSetting->setValue(SETTING_KEY_BOTTOM_SUB_PROTECT, bottomSubtitleProtect);
    refreshDMRect();
}

void DanmuRender::setTopSubtitleProtect(bool topOn)
{
    topSubtitleProtect = topOn;
    GlobalObjects::appSetting->setValue(SETTING_KEY_TOP_SUB_PROTECT, topSubtitleProtect);
    refreshDMRect();
}

void DanmuRender::setDisplayArea(int index)
{
    displayAreaIndex = index;
    displayAreaRatio = rangeMapping[index];
    GlobalObjects::appSetting->setValue(SETTING_KEY_DISPLAY_AREA, index);
    refreshDMRect();
}

void DanmuRender::setFontSize(int pt)
{
    fontSizeTable[0] = pt;
    fontSizeTable[1] = fontSizeTable[0]/1.5;
    fontSizeTable[2] = fontSizeTable[0]*1.5;
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMU_FONT_SIZE, pt);
}

void DanmuRender::setBold(bool bold)
{
    danmuStyle.bold = bold;
    emit danmuStyleChanged();
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMU_BOLD, bold);
}

void DanmuRender::setGlow(bool on)
{
    danmuStyle.glow = on;
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMU_GLOW, on);
}

void DanmuRender::setOpacity(float opacity)
{
    danmuOpacity = qBound(0.f, opacity, 1.f);
    liveDanmuListModel->setAlpha(danmuOpacity);
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMU_ALPHA, (int)(opacity * 100));

}

void DanmuRender::setFontFamily(const QString &family)
{
    danmuStyle.fontFamily = family;
    liveDanmuListModel->setFontFamily(danmuStyle.fontFamily);
    emit danmuStyleChanged();
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMU_FONT, family);
}

void DanmuRender::setSpeed(float speed)
{
    static_cast<RollLayout *>(layout_table[DanmuComment::DanmuType::Rolling])->setSpeed(speed);
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMU_SPEED, (int)speed);
}

float DanmuRender::getSpeed() const
{
    return static_cast<RollLayout *>(layout_table[DanmuComment::DanmuType::Rolling])->getSpeed();
}

void DanmuRender::setStrokeWidth(float width)
{
    danmuStyle.strokeWidth = width;
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMU_STROKE, (int)(width * 10));
}

void DanmuRender::setRandomSize(bool randomSize)
{
    danmuStyle.randomSize = randomSize;
    GlobalObjects::appSetting->setValue(SETTING_KEY_DANMU_RANDOM_SIZE, randomSize);
}

void DanmuRender::setMaxDanmuCount(int count)
{
    maxCount = count;
    GlobalObjects::appSetting->setValue(SETTING_KEY_MAX_COUNT, count);
}

void DanmuRender::setMergeCountPos(int pos)
{
    danmuStyle.mergeCountPos = pos;
    liveDanmuListModel->setMergeCountPos(pos);
    GlobalObjects::appSetting->setValue(SETTING_KEY_MERGE_COUNT_POS, pos);
}

void DanmuRender::setEnlargeMerged(bool enlarge)
{
    danmuStyle.enlargeMerged = enlarge;
    GlobalObjects::appSetting->setValue(SETTING_KEY_ENLARGE_MERGED, enlarge);
    liveDanmuListModel->setEnlargeMerged(enlarge);
}

void DanmuRender::setLiveMode(bool on, int onlyRolling)
{
    enableLiveMode = on;
    GlobalObjects::appSetting->setValue(SETTING_KEY_ENABLE_LIVE_MODE, on);
    if (onlyRolling != -1)
    {
        liveModeOnlyRolling = onlyRolling;
        GlobalObjects::appSetting->setValue(SETTING_KEY_LIVE_MODE_ONLY_ROLLING, liveModeOnlyRolling);
    }
}

void DanmuRender::setDenseLevel(int level)
{
    dense = level;
    GlobalObjects::appSetting->setValue(SETTING_KEY_DENSE_LEVEL, level);
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


