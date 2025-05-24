#include "cacheworker.h"
#include <QtConcurrent>
#ifdef TEXTURE_MAIN_THREAD
#include "globalobjects.h"
#include "Play/Video/mpvplayer.h"
#endif
#ifdef QT_DEBUG
#include "Common/counter.h"
#endif
extern QOpenGLContext *danmuTextureContext;
extern QOffscreenSurface *surface;

// in src/qtbase/src/widgets/effects/qpixmapfilter.cpp
extern Q_DECL_IMPORT void qt_blurImage(QImage &blurImage, qreal radius, bool quality, int transposed = 0);

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
    changeDanmuStyle();
    danmuStrokePen.setWidthF(danmuStyle->strokeWidth);
    danmuStrokePen.setJoinStyle(Qt::RoundJoin);
    danmuStrokePen.setCapStyle(Qt::RoundCap);
}

QImage CacheWorker::createPreviewImage(const DanmuStyle *style, const DanmuComment *comment)
{
    QFont danmuFont;
    danmuFont.setFamily(style->fontFamily);
    danmuFont.setBold(style->bold);
    if (style->randomSize)
    {
        danmuFont.setPointSize(QRandomGenerator::global()->bounded(style->fontSizeTable[DanmuComment::FontSizeLevel::Small],
            style->fontSizeTable[DanmuComment::FontSizeLevel::Large]));
    }
    else
    {
        danmuFont.setPointSize(style->fontSizeTable[comment->fontSizeLevel]);
    }

    QFontMetrics metrics(danmuFont);
    int left = qAbs(metrics.leftBearing(comment->text.front()));
    QSize imgSize = metrics.size(0, comment->text) + QSize(style->strokeWidth*2 + left, style->strokeWidth);
    int py = qAbs((imgSize.height() - metrics.height()) / 2 + metrics.ascent());

    QPainterPath path;
    path.addText(left + style->strokeWidth, py, danmuFont, comment->text);

    QImage img(imgSize * style->devicePixelRatioF, QImage::Format_ARGB32_Premultiplied);
    img.setDevicePixelRatio(style->devicePixelRatioF);
    img.fill(Qt::transparent);
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::TextAntialiasing);
    if (style->strokeWidth > 0)
    {
        QPen danmuStrokePen;
        danmuStrokePen.setWidthF(style->strokeWidth);
        danmuStrokePen.setJoinStyle(Qt::RoundJoin);
        danmuStrokePen.setCapStyle(Qt::RoundCap);
        danmuStrokePen.setColor(comment->color == 0x000000 ? Qt::white : Qt::black);
        painter.strokePath(path, danmuStrokePen);
        painter.drawPath(path);
    }
    int r = (comment->color>>16)&0xff, g = (comment->color>>8)&0xff, b = comment->color&0xff;
    if (style->randomColor)
    {
        r = QRandomGenerator::global()->bounded(0, 256);
        g = QRandomGenerator::global()->bounded(0, 256);
        b = QRandomGenerator::global()->bounded(0, 256);
    }
    painter.fillPath(path, QColor(r, g, b));
    if (style->glow)
    {
        QImage tmp(img);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(img.rect(), QColor(0, 0, 0));
        qt_blurImage(img, style->glowRadius, true);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, tmp);
    }
    painter.end();
    return img;
}

void CacheWorker::cleanCache()
{
#ifdef QT_DEBUG
    qDebug()<<"clean start, items:"<<danmuCache.size();
    QElapsedTimer timer;
    timer.start();
    int cacheCount = 0;
    int rmTexture = 0;
#endif
    for(auto iter=danmuCache.begin();iter!=danmuCache.end();)
    {
        Q_ASSERT(iter.value()->useCount >= 0);
        if(iter.value()->useCount==0)
        {
            if(iter.value()->cacheFlag)
            {
                iter.value()->cacheFlag = false;
                ++iter;
#ifdef QT_DEBUG
                ++cacheCount;
#endif
            }
            else
            {
                --textureRef[iter.value()->texture];
                delete iter.value();
                iter=danmuCache.erase(iter);
            }
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
#ifdef QT_DEBUG
            ++rmTexture;
#endif
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
    qDebug()<<"clean done: "<<timer.elapsed()<<"ms, left item: "<<danmuCache.size()<<", cache: "<<cacheCount;
    qDebug()<<"remove texture: "<<rmTexture<<", left texture: "<<textureRef.size();
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
    drawInfo->useCount = 0;
    drawInfo->cacheFlag = false;
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
    QImage *img = new QImage(imgSize, QImage::Format_ARGB32_Premultiplied);
    img->fill(Qt::transparent);
    QPainter painter(img);
    painter.setRenderHint(QPainter::Antialiasing);
    int r=(comment->color>>16)&0xff,g=(comment->color>>8)&0xff,b=comment->color&0xff;
    if (danmuStyle->randomColor)
    {
        r = QRandomGenerator::global()->bounded(0, 256);
        g = QRandomGenerator::global()->bounded(0, 256);
        b = QRandomGenerator::global()->bounded(0, 256);
    }
    if(strokeWidth>0)
    {
        danmuStrokePen.setColor(comment->color==0x000000?Qt::white:Qt::black);
        painter.strokePath(path,danmuStrokePen);
        painter.drawPath(path);
    }
    painter.fillPath(path,QBrush(QColor(r,g,b)));
    if(danmuStyle->glow)
    {
        QImage tmp(*img);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(img->rect(), QColor(0, 0, 0));
        qt_blurImage(*img, danmuStyle->glowRadius, true);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        painter.drawImage(0, 0, tmp);
    }
    painter.end();

    midInfo.img=img;
    midInfo.drawInfo=drawInfo;
}

void CacheWorker::createTexture(QVector<CacheMiddleInfo> &midInfo)
{
    int cachePos = 0;
    std::sort(midInfo.begin(), midInfo.end(), [](const CacheMiddleInfo &c1, const CacheMiddleInfo &c2){
        return c1.drawInfo->height > c2.drawInfo->height;
    });
    int textureWidth = 0;
    for (auto& m : midInfo)
    {
        textureWidth = qMax(textureWidth, m.drawInfo->width);
    }
    textureWidth = textureWidth>2048?2048:powerOf2GE(textureWidth);
    using DanmuLevel = QPair<int, int>;  // h, x
    while (cachePos < midInfo.size())
    {
        QVector<DanmuLevel> levels{{midInfo[cachePos].drawInfo->height, 0}};
        levels.reserve(32);
        int endPos = cachePos;
        for (; endPos < midInfo.size(); ++endPos)
        {
            auto& mInfo = midInfo[endPos];
            bool insert = false;
            int y = 0;
            for (auto& level : levels)
            {
                if (level.first >= mInfo.drawInfo->height && textureWidth - level.second > mInfo.drawInfo->width)
                {
                    mInfo.texX = level.second;
                    mInfo.texY = y;
                    level.second += mInfo.drawInfo->width;
                    insert = true;
                    break;
                }
                y += level.first;
            }
            if (!insert)
            {
                if (y + mInfo.drawInfo->height > 2048) break;
                if (y + mInfo.drawInfo->height > 1024)
                {
                    if (mInfo.drawInfo->height * (midInfo.size() - endPos) < 512) break;
                }
                mInfo.texX = 0;
                mInfo.texY = y;
                levels.append({ mInfo.drawInfo->height, mInfo.drawInfo->width });
            }
        }
        assert(endPos != cachePos);
        int textureHeight = 0;
        for (const auto& l : levels) textureHeight += l.first;
        textureHeight = textureHeight > 2048 ? 2048 : powerOf2GE(textureHeight);
#ifdef QT_DEBUG
        qDebug() << "texture: start: " << cachePos << ", end: " << endPos << ", total: " << midInfo.size();
        qDebug() << "texture: size: " << textureWidth << ", " << textureHeight;
        Counter::instance()->countValue("texture.num_dm", midInfo.size());
        Counter::instance()->countValue("texture.width", textureWidth);
        Counter::instance()->countValue("texture.height", textureHeight);
        for (int i = cachePos; i < endPos; ++i)
        {
            auto& mInfo = midInfo[i];
            assert(mInfo.texX < textureWidth && mInfo.texY < textureHeight);
        }
        static bool ft = true;
        if (ft)
        {
            QImage test(textureWidth, textureHeight, QImage::Format_ARGB32);
            QPainter painter(&test);
            painter.setRenderHint(QPainter::Antialiasing);
            ft = false;
            for (int i = cachePos; i < endPos; ++i)
            {
                auto& mInfo = midInfo[i];
                painter.drawImage(mInfo.texX, mInfo.texY, *mInfo.img);
            }
            test.save("texture_test.jpg");
        }
#endif
#ifdef TEXTURE_MAIN_THREAD
    QMetaObject::invokeMethod(GlobalObjects::mpvplayer,[this,&midInfo,textureWidth,textureHeight,endPos](){
#endif
        danmuTextureContext->makeCurrent(surface);
        QOpenGLFunctions *glFuns=danmuTextureContext->functions();
        if (!init)
        {
            glFuns->initializeOpenGLFunctions();
            init = true;
        }
        GLuint texture;
        glFuns->glGenTextures(1, &texture);
        glFuns->glBindTexture(GL_TEXTURE_2D, texture);
        glFuns->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glFuns->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        textureRef.insert(texture, endPos - cachePos);

        for (int i = cachePos; i < endPos; ++i)
        {
            auto& mInfo = midInfo[i];
            mInfo.drawInfo->texture = texture;
            glFuns->glTexSubImage2D(GL_TEXTURE_2D, 0, mInfo.texX, mInfo.texY, mInfo.drawInfo->width
                , mInfo.drawInfo->height, GL_RGBA, GL_UNSIGNED_BYTE, mInfo.img->bits());
            mInfo.drawInfo->l = ((GLfloat)mInfo.texX) / textureWidth;
            mInfo.drawInfo->r = ((GLfloat)mInfo.texX + mInfo.drawInfo->width) / textureWidth;
            if (mInfo.drawInfo->r > 1)mInfo.drawInfo->r = 1;
            mInfo.drawInfo->t = ((GLfloat)mInfo.texY) / textureHeight;
            mInfo.drawInfo->b = ((GLfloat)mInfo.texY + mInfo.drawInfo->height) / textureHeight;
            if (mInfo.drawInfo->b > 1)mInfo.drawInfo->b = 1;
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
        cachePos = endPos;
    }
}

void CacheWorker::beginCache(QVector<DrawTask> *danmus)
{
#ifdef QT_DEBUG
    qDebug()<<"cache start---";
    QElapsedTimer timer, stepTimer;
    timer.start();
    stepTimer.start();
#endif
    QStringList hashList;
    QSet<QString> tmpHash;
    QVector<CacheMiddleInfo> mInfoList;
    for (auto& dm : *danmus)
    {
        QString hash_str(QCryptographicHash::hash(QString("%1%2%3%4")
            .arg(dm.comment->text
                , QString::number(dm.comment->color)
                , QString::number(danmuStyle->fontSizeTable[dm.comment->fontSizeLevel])
                , dm.comment->mergedList ? QString::number(dm.comment->mergedList->count()) : "0").toUtf8()
            , QCryptographicHash::Md5).toHex());
        hashList << hash_str;
        if (!danmuCache.contains(hash_str) && !tmpHash.contains(hash_str))
        {
            CacheMiddleInfo mInfo;
            mInfo.hash = hash_str;
            mInfo.comment = dm.comment.data();
            mInfoList.append(mInfo);
            tmpHash.insert(hash_str);
        }
    }
#ifdef QT_DEBUG
    Counter::instance()->countValue("cache.step1.prepare", stepTimer.restart());
    Counter::instance()->countValue("cache.count", mInfoList.size());
    Counter::instance()->countValue("cache.dmcount", danmus->size());
    Counter::instance()->countValue("cache.miss_rate", ((double)mInfoList.size() / (double)danmus->size()) * 1000);
#endif
	if (!mInfoList.isEmpty())
	{
		QtConcurrent::blockingMap(mInfoList, std::bind(&CacheWorker::createImage, this, std::placeholders::_1));
#ifdef QT_DEBUG
    Counter::instance()->countValue("cache.step2.img", stepTimer.restart());
#endif
		createTexture(mInfoList);
#ifdef QT_DEBUG
    Counter::instance()->countValue("cache.step3.texture", stepTimer.restart());
#endif
		for (auto &mInfo : mInfoList)
		{
			Q_ASSERT(!danmuCache.contains(mInfo.hash));
			danmuCache.insert(mInfo.hash, mInfo.drawInfo);
		}
	}
    int i=0;
    for (auto& dm : *danmus)
    {
        DanmuDrawInfo* drawInfo(danmuCache.value(hashList[i++], nullptr));
        Q_ASSERT(drawInfo);
        if (dm.isCurrent)
        {
            drawInfo->useCount++;
            drawInfo->cacheFlag = false;
        }
        else
        {
            drawInfo->cacheFlag = true;
        }
        dm.drawInfo = drawInfo;
    }
#ifdef QT_DEBUG
    qDebug()<<"cache end, time: "<<timer.elapsed()<<"ms";
    Counter::instance()->countValue("cache.step4.end", stepTimer.elapsed());
#endif
    emit cacheDone(danmus);
}

void CacheWorker::changeRefCount(QVector<DanmuDrawInfo *> *descList)
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
