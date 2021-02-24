#include "cacheworker.h"
#include <QtConcurrent>
#ifdef TEXTURE_MAIN_THREAD
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#endif
extern QOpenGLContext *danmuTextureContext;
extern QOffscreenSurface *surface;

namespace
{
    int powerOf2GE(int x)
    {
        --x;
        x = x | (x >> 1);
        x = x | (x >> 2);
        x = x | (x >> 4);
        x = x | (x >> 8);
        x = x | (x >> 16);
        return x + 1;
    }
}
CacheWorker::CacheWorker(const DanmuStyle *style):danmuStyle(style)
{
    danmuFont.setFamily(danmuStyle->fontFamily);
    danmuStrokePen.setWidthF(danmuStyle->strokeWidth);
    danmuStrokePen.setJoinStyle(Qt::RoundJoin);
    danmuStrokePen.setCapStyle(Qt::RoundCap);
}

void CacheWorker::cleanCache()
{
#ifdef QT_DEBUG
    qDebug()<<"clean start, items:"<<danmuCache.size();
    QElapsedTimer timer;
    timer.start();
#endif
    for(auto iter=danmuCache.begin();iter!=danmuCache.end();)
    {
        Q_ASSERT(iter.value()->useCount >= 0);
        if(iter.value()->useCount==0)
        {
            --textureRef[iter.value()->texture];
            delete iter.value();
            iter=danmuCache.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
#ifdef TEXTURE_MAIN_THREAD
    QMetaObject::invokeMethod(GlobalObjects::mpvplayer,[this](){
#endif
    if(!danmuTextureContext->makeCurrent(surface)) return;
    QOpenGLFunctions *glFuns=danmuTextureContext->functions();
    for(auto iter=textureRef.begin();iter!=textureRef.end();)
    {
        Q_ASSERT(iter.value()>= 0);
        if(iter.value()==0)
        {
            glFuns->glDeleteTextures(1,&iter.key());
            iter=textureRef.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    danmuTextureContext->doneCurrent();
#ifdef TEXTURE_MAIN_THREAD
    },Qt::BlockingQueuedConnection);
#endif
#ifdef QT_DEBUG
    qDebug()<<"clean done:"<<timer.elapsed()<<"ms, left item:"<<danmuCache.size();
#endif
}

void CacheWorker::createImage(CacheMiddleInfo &midInfo)
{
    DanmuComment *comment=midInfo.comment;
    QFont danmuFont(this->danmuFont);
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
    QPen danmuStrokePen;
    danmuStrokePen.setWidthF(danmuStyle->strokeWidth);
    int strokeWidth=danmuStyle->strokeWidth;
    int left=qAbs(metrics.leftBearing(comment->text.front()));

    QSize textSize;
    int pointSize=danmuFont.pointSize();
    int minPointSize=pointSize/2;
    //If the width is greater than 2048, then adjust the font size
    //but the font size cannot be less than half of the previous point size
    do
    {
        danmuFont.setPointSize(pointSize--);
        QFontMetrics tMetrice(danmuFont);
        textSize = tMetrice.size(0, comment->text);
        metrics.swap(tMetrice);
    }while(textSize.width()>2048 && pointSize>minPointSize);

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
    if(imgSize.width()>2048)imgSize.rwidth()=2048;
    if(imgSize.height()>2048)imgSize.rheight()=2048;

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
    QImage *img=new QImage(imgSize, QImage::Format_ARGB32);
    img->fill(Qt::transparent);
    QPainter painter(img);
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

    midInfo.img=img;
    midInfo.drawInfo=drawInfo;
}

void CacheWorker::createTexture(QList<CacheMiddleInfo> &midInfo)
{
    int textureWidth=0,textureHeight;
    for(auto &mInfo:midInfo)
    {
        if(mInfo.drawInfo->width>textureWidth) textureWidth=mInfo.drawInfo->width;
    }
    textureWidth = textureWidth>2048?2048:powerOf2GE(textureWidth);
    int x=0,y=0,rowHeight=0;
    for(auto &mInfo:midInfo)
    {
        if(textureWidth-x>mInfo.drawInfo->width)
        {
            mInfo.texX=x;
            mInfo.texY=y;
            x+=mInfo.drawInfo->width;
            if(mInfo.drawInfo->height>rowHeight) rowHeight=mInfo.drawInfo->height;
        }
        else
        {
            x=0;
            y+=rowHeight;
            rowHeight=mInfo.drawInfo->height;
            mInfo.texX=x;
            mInfo.texY=y;
			x += mInfo.drawInfo->width;
        }
    }
    textureHeight=y+rowHeight;
    textureHeight = textureHeight>2048?2048:powerOf2GE(textureHeight);
#ifdef TEXTURE_MAIN_THREAD
    QMetaObject::invokeMethod(GlobalObjects::mpvplayer,[this,&midInfo,textureWidth,textureHeight](){
#endif
    danmuTextureContext->makeCurrent(surface);
    QOpenGLFunctions *glFuns=danmuTextureContext->functions();
    if(!init)
    {
        glFuns->initializeOpenGLFunctions();
        init = true;
    }
    GLuint texture;
    glFuns->glGenTextures(1, &texture);
    glFuns->glBindTexture(GL_TEXTURE_2D, texture);
    glFuns->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glFuns->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    textureRef.insert(texture,midInfo.size());

    for(auto &mInfo:midInfo)
    {
        mInfo.drawInfo->texture=texture;
        glFuns->glTexSubImage2D(GL_TEXTURE_2D,0,mInfo.texX,mInfo.texY,mInfo.drawInfo->width
                                ,mInfo.drawInfo->height,GL_RGBA,GL_UNSIGNED_BYTE,mInfo.img->bits());
        mInfo.drawInfo->l=((GLfloat)mInfo.texX)/textureWidth;
        mInfo.drawInfo->r=((GLfloat)mInfo.texX+mInfo.drawInfo->width)/textureWidth;
        if(mInfo.drawInfo->r>1)mInfo.drawInfo->r=1;
        mInfo.drawInfo->t=((GLfloat)mInfo.texY)/textureHeight;
        mInfo.drawInfo->b=((GLfloat)mInfo.texY+mInfo.drawInfo->height)/textureHeight;
        if(mInfo.drawInfo->b>1)mInfo.drawInfo->b=1;
        delete mInfo.img;
    }
    glFuns->glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glFuns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glFuns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFuns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glFuns->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    danmuTextureContext->doneCurrent();
#ifdef TEXTURE_MAIN_THREAD
    },Qt::BlockingQueuedConnection);
#endif
}

void CacheWorker::beginCache(QList<DrawTask> *danmus)
{
#ifdef QT_DEBUG
    qDebug()<<"cache start---";
    QElapsedTimer timer;
    timer.start();
    qint64 etime=0;
#endif
    QStringList hashList;
    QSet<QString> tmpHash;
    QList<CacheMiddleInfo> mInfoList;
    for(auto &dm:*danmus)
    {
        QString hash_str(QCryptographicHash::hash(QString("%1%2%3%4")
                         .arg(dm.comment->text
                              ,QString::number(dm.comment->color)
                              ,QString::number(danmuStyle->fontSizeTable[dm.comment->fontSizeLevel])
                         ,dm.comment->mergedList?QString::number(dm.comment->mergedList->count()):"0").toUtf8()
                         ,QCryptographicHash::Md5).toHex());
        hashList<<hash_str;
        if(!danmuCache.contains(hash_str) && !tmpHash.contains(hash_str))
        {
            CacheMiddleInfo mInfo;
            mInfo.hash=hash_str;
            mInfo.comment=dm.comment.data();
            mInfoList.append(mInfo);
            tmpHash.insert(hash_str);
        }
    }
	if (!mInfoList.isEmpty())
	{
		QtConcurrent::blockingMap(mInfoList, std::bind(&CacheWorker::createImage, this, std::placeholders::_1));
		createTexture(mInfoList);
		for (auto &mInfo : mInfoList)
		{
			Q_ASSERT(!danmuCache.contains(mInfo.hash));
			danmuCache.insert(mInfo.hash, mInfo.drawInfo);
		}
	}
    int i=0;
    for(auto &dm:*danmus)
    {
         DanmuDrawInfo *drawInfo(danmuCache.value(hashList[i++],nullptr));
         Q_ASSERT(drawInfo);
         if(dm.isCurrent) drawInfo->useCount++;
         dm.drawInfo=drawInfo;
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
    {
        drawInfo->useCount--;
    }
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
