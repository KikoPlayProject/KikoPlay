#include "cacheworker.h"

extern QOpenGLContext *danmuTextureContext;
extern QSurface *surface;

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
    {
        danmuFont.setPointSize(QRandomGenerator::global()->
                               bounded(danmuStyle->fontSizeTable[DanmuComment::FontSizeLevel::Small],
                               danmuStyle->fontSizeTable[DanmuComment::FontSizeLevel::Large]));
    }
    else if(comment->mergedList && danmuStyle->enlargeMerged)
    {
        float enlargeRate(qBound(1.f,log2f(comment->mergedList->count()+1)/2,2.5f));
        danmuFont.setPointSize(danmuStyle->fontSizeTable[DanmuComment::FontSizeLevel::Normal]*enlargeRate);
    }
    else
    {
        danmuFont.setPointSize(danmuStyle->fontSizeTable[comment->fontSizeLevel]);
    }
    QFontMetrics metrics(danmuFont);
    danmuStrokePen.setWidthF(danmuStyle->strokeWidth);
    int strokeWidth=danmuStyle->strokeWidth;
    int left=qAbs(metrics.leftBearing(comment->text.front()));

    QSize textSize(metrics.size(0, comment->text));
    QSize imgSize=textSize+QSize(strokeWidth*2+left,strokeWidth);
    int mergeCountWidth=0;
    if(comment->mergedList && danmuStyle->mergeCountPos>0)
    {
        QFont tmpFont(danmuFont);
        tmpFont.setPointSize(tmpFont.pointSize()/2);
        QFontMetrics metrics(tmpFont);
        mergeCountWidth=metrics.size(0,QString("[%1]").arg(comment->mergedList->count())).width();
        imgSize.rwidth()+=mergeCountWidth;
    }
    QImage img(imgSize, QImage::Format_ARGB32);

    DanmuDrawInfo *drawInfo=new DanmuDrawInfo;
    drawInfo->useCount=0;
    drawInfo->height=imgSize.height();
    drawInfo->width=imgSize.width();
    //drawInfo->img=img;

    QPainterPath path;
    QStringList multilines(comment->text.split('\n'));
    int py = qAbs((imgSize.height() - metrics.height()*multilines.size()) / 2 + metrics.ascent());
    int i=0;
    for(const QString &line:multilines)
    {
        if(i==0 && danmuStyle->mergeCountPos==1 && comment->mergedList)
        {
            int sz=danmuFont.pointSize();
            danmuFont.setPointSize(sz/2);
            path.addText(left+strokeWidth,py,danmuFont,QString("[%1]").arg(comment->mergedList->count()));
            danmuFont.setPointSize(sz);
            path.addText(left+strokeWidth+mergeCountWidth,py,danmuFont,line);
        }
        else if(i==multilines.count()-1 && danmuStyle->mergeCountPos==2 && comment->mergedList)
        {
            path.addText(left+strokeWidth,py+i*metrics.height(),danmuFont,line);
            int sz=danmuFont.pointSize();
            danmuFont.setPointSize(sz/2);
            path.addText(left+strokeWidth+textSize.width(),py+i*metrics.height(),danmuFont,QString("[%1]").arg(comment->mergedList->count()));
            danmuFont.setPointSize(sz);
        }
        else
        {
            path.addText(left+strokeWidth,py+i*metrics.height(),danmuFont,line);
        }
        ++i;
    }
    img.fill(Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    int r=(comment->color>>16)&0xff,g=(comment->color>>8)&0xff,b=comment->color&0xff;
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
         QString hash_str(QString("%1%2%3%4")
                          .arg(dm.first->text
                               ,QString::number(dm.first->color)
                               ,QString::number(danmuStyle->fontSizeTable[dm.first->fontSizeLevel])
                          ,dm.first->mergedList?QString::number(dm.first->mergedList->count()):"0"));
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
