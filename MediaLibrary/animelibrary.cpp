#include "animelibrary.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QPixmap>
#include <QCollator>
#include "globalobjects.h"
#include "animeworker.h"
#include "Common/network.h"
#include "Common/threadtask.h"
#include "animemodel.h"
#include "labelmodel.h"
#define AnimeRole Qt::UserRole+1
namespace
{
    static QCollator comparer;
    struct
    {
        inline bool operator()(const Episode &ep1, const Episode &ep2) const
        {
            return comparer.compare(ep1.name,ep2.name)>=0?false:true;
        }
    } epCompare;
}
AnimeLibrary::AnimeLibrary(QObject *parent):QObject(parent)
{
    comparer.setNumericMode(true);
    animeWorker=new AnimeWorker();
    animeWorker->moveToThread(GlobalObjects::workThread);
    QObject::connect(animeWorker,&AnimeWorker::addAnime,this,&AnimeLibrary::addAnime);
    QObject::connect(this,(void (AnimeLibrary:: *)(const QString &, const QString &,const QString &))&AnimeLibrary::tryAddAnime,
                     animeWorker,(void (AnimeWorker:: *)(const QString &, const QString &,const QString &))&AnimeWorker::addAnimeInfo);
    QObject::connect(this,(void (AnimeLibrary:: *)(const QString &, int))&AnimeLibrary::tryAddAnime,
                     animeWorker,(void (AnimeWorker:: *)(const QString &, int))&AnimeWorker::addAnimeInfo);
    QObject::connect(animeWorker,&AnimeWorker::newTagDownloaded,this,&AnimeLibrary::addTagsTo);
    QObject::connect(animeWorker,&AnimeWorker::downloadDetailMessage,this,&AnimeLibrary::downloadDetailMessage);
    animeModel=new AnimeModel(this);
    labelModel=new LabelModel(this);
}

AnimeLibrary::~AnimeLibrary()
{
    animeModel->deleteLater();
    labelModel->deleteLater();
}


void AnimeLibrary::addToLibrary(const QString &animeName, const QString &epName, const QString &path)
{
    emit tryAddAnime(animeName,epName,path);
}

void AnimeLibrary::addToLibrary(const QString &animeName, int bgmId)
{
    emit tryAddAnime(animeName,bgmId);
}

Anime *AnimeLibrary::downloadDetailInfo(Anime *anime, int bangumiId, QString *errorInfo)
{
    QString errInfo; 
    QEventLoop eventLoop;
    Anime *resultAnime =anime;
    QString oldDate(anime->date.left(7));
	bool merged = false;
    QObject::connect(animeWorker,&AnimeWorker::downloadDone, &eventLoop,[&errInfo,&eventLoop](const QString &err){
        errInfo=err;
        eventLoop.quit();
    });
    QObject::connect(animeWorker,&AnimeWorker::mergeAnime, &eventLoop, [this,&eventLoop,&resultAnime,&merged](Anime *oldAnime,Anime *newAnime){
        resultAnime=newAnime;
        emit removeOldAnime(oldAnime);
		merged = true;
        eventLoop.quit();
    });
    QMetaObject::invokeMethod(animeWorker,[anime,bangumiId,this](){
        animeWorker->downloadDetailInfo(anime,bangumiId);
    },Qt::QueuedConnection);
    eventLoop.exec();
    QString animeDate(resultAnime->date.left(7));
    if(!merged && !animeDate.isEmpty())
    {
        emit addTimeLabel(animeDate,oldDate);
    }
    *errorInfo=errInfo;
    return resultAnime;
}

void AnimeLibrary::refreshEpPlayTime(const QString &title, const QString &path)
{
    QMetaObject::invokeMethod(animeWorker,[this,&title,&path](){
        animeWorker->updatePlayTime(title,path);
    },Qt::QueuedConnection);
}

QString AnimeLibrary::updateCrtImage(Anime *anime, Character *crt)
{
    QString errInfo;
    if(anime->bangumiID==-1 || crt->imgURL.isEmpty()) return errInfo;
    try
    {
        QByteArray img(Network::httpGet(crt->imgURL,QUrlQuery()));
        crt->image=img;
        QMetaObject::invokeMethod(animeWorker,[this,anime,crt](){
            animeWorker->updateCrtImage(anime->title,crt);
        },Qt::QueuedConnection);
    }
    catch (Network::NetworkError &err)
    {
        errInfo=err.errorInfo;
    }
    return errInfo;
}

void AnimeLibrary::loadTags(QMap<QString, QSet<QString> > &tagMap, QMap<QString, int> &timeMap)
{
    QEventLoop eventLoop;
    QObject::connect(animeWorker,&AnimeWorker::loadLabelInfoDone, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(animeWorker,[this,&tagMap,&timeMap](){
        animeWorker->loadLabelInfo(tagMap,timeMap);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

void AnimeLibrary::deleteTag(const QString &tag, const QString &animeTitle)
{
    QMetaObject::invokeMethod(animeWorker,[this,tag,animeTitle](){
        animeWorker->deleteTag(tag,animeTitle);
    },Qt::QueuedConnection);
    if(!tag.isEmpty() && !animeTitle.isEmpty())
        emit removeTagFrom(animeTitle,tag);
}

void AnimeLibrary::addTags(Anime *anime, const QStringList &tags)
{
    emit addTagsTo(anime->title,tags);
}

void AnimeLibrary::saveTags(const QString &title, const QStringList &tags)
{
    QMetaObject::invokeMethod(animeWorker,[this,title,tags](){
        animeWorker->saveTags(title,tags);
    },Qt::QueuedConnection);
}

void AnimeLibrary::downloadTags(Anime *anime, QStringList &tagList, QString *errorInfo)
{
    QString errInfo;
    QEventLoop eventLoop;
    QObject::connect(animeWorker,&AnimeWorker::downloadTagDone, &eventLoop,[&errInfo,&eventLoop](const QString &err){
        errInfo=err;
        eventLoop.quit();
    });
    QMetaObject::invokeMethod(animeWorker,[anime,&tagList,this](){
        animeWorker->downloadTags(anime->bangumiID,tagList);
    },Qt::QueuedConnection);
    eventLoop.exec();
    *errorInfo=errInfo;
}

void AnimeLibrary::fetchAnimes(QList<Anime *> &animeList, int curOffset, int limitCount)
{
    QEventLoop eventLoop;
    QObject::connect(animeWorker,&AnimeWorker::loadDone, &eventLoop,&QEventLoop::quit);
    QMetaObject::invokeMethod(animeWorker,[this,&animeList,curOffset,limitCount](){
        animeWorker->loadAnimes(&animeList,curOffset,limitCount);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

void AnimeLibrary::deleteAnime(Anime *anime)
{
    QEventLoop eventLoop;
    QObject::connect(animeWorker,&AnimeWorker::deleteDone, &eventLoop,&QEventLoop::quit);
    emit removeTags(anime->title,anime->date.left(7));
    QMetaObject::invokeMethod(animeWorker,[this,anime](){
        animeWorker->deleteAnime(anime);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

void AnimeLibrary::fillAnimeInfo(Anime *anime)
{
    if(anime->eps.count()==0)
    {
        QSqlQuery query(QSqlDatabase::database("Bangumi_M"));
        query.prepare("select * from eps where Anime=?");
        query.bindValue(0,anime->title);
        query.exec();
        int nameNo=query.record().indexOf("Name"),
            localFileNo=query.record().indexOf("LocalFile"),
            lastPlayTimeNo=query.record().indexOf("LastPlayTime");
        while (query.next())
        {
            Episode ep;
            ep.name=query.value(nameNo).toString();
            ep.localFile=query.value(localFileNo).toString();
            ep.lastPlayTime=query.value(lastPlayTimeNo).toString();
            anime->eps.append(ep);
        }
        std::sort(anime->eps.begin(), anime->eps.end(), epCompare);
    }
    if(!anime->loadCrtImage && anime->bangumiID!=-1)
    {
        QSqlQuery query(QSqlDatabase::database("Bangumi_M"));
        query.prepare("select * from character_image where Anime=?");
        query.bindValue(0,anime->title);
        query.exec();
        int cidNo=query.record().indexOf("CBangumiID"),
            urlNo=query.record().indexOf("ImageURL"),
            imgNo=query.record().indexOf("Image");
        while (query.next())
        {
            for(Character &crt:anime->characters)
            {
                if(crt.bangumiID==query.value(cidNo).toInt())
                {
                    crt.imgURL=query.value(urlNo).toString();
                    crt.image=query.value(imgNo).toByteArray();
                    break;
                }
            }
        }
        anime->loadCrtImage=true;
    }
}

int AnimeLibrary::fetchAnimeCaptures(const QString &animeName, QList<CaptureItem *> &captureList, int offset, int limit)
{
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&animeName,&captureList,offset,limit](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("select Time,Info,Thumb from capture where Anime=? order by Time desc limit ? offset ?");
        query.bindValue(0,animeName);
        query.bindValue(1,limit);
        query.bindValue(2,offset);
        query.exec();
        int timeNo=query.record().indexOf("Time"),
            infoNo=query.record().indexOf("Info"),
            thumbNo=query.record().indexOf("Thumb");
        int count=0;
        while (query.next())
        {
            CaptureItem *captrue=new CaptureItem;
            captrue->timeId=query.value(timeNo).toLongLong();
            captrue->info=query.value(infoNo).toString();
            captrue->thumb=query.value(thumbNo).toByteArray();
            captureList.append(captrue);
            ++count;
        }
        return count;
    }).toInt();
}

QPixmap AnimeLibrary::getCapture(qint64 timeId)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
    query.prepare("select Image from capture where Time=?");
    query.bindValue(0,timeId);
    query.exec();
    QPixmap image;
    if(query.first())
    {
        image.loadFromData(query.value(0).toByteArray());
    }
    return image;
}

void AnimeLibrary::saveCapture(const QString &animeName, const QString &info, const QImage &image)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([animeName,info,image](){
        QByteArray imgBytes;
        QBuffer bufferImage(&imgBytes);
        bufferImage.open(QIODevice::WriteOnly);
        image.save(&bufferImage, "JPG");

        QImage &&thumb=image.scaled(200,112,Qt::AspectRatioMode::KeepAspectRatioByExpanding);
        QByteArray thumbBytes;
        QBuffer bufferThumb(&thumbBytes);
        bufferThumb.open(QIODevice::WriteOnly);
        thumb.save(&bufferThumb, "PNG");

        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("insert into capture(Time,Anime,Info,Thumb,Image) values(?,?,?,?,?)");
        query.bindValue(0,QDateTime::currentDateTime().toSecsSinceEpoch());
        query.bindValue(1,animeName);
        query.bindValue(2,info);
        query.bindValue(3,thumbBytes);
        query.bindValue(4,imgBytes);
        query.exec();
    });

}

void AnimeLibrary::deleteCapture(qint64 timeId)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([timeId](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("delete from capture where Time=?");
        query.bindValue(0,timeId);
        query.exec();
    });
}

int AnimeLibrary::getTotalAnimeCount()
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_M"));
    query.exec("select count(*) from anime");
    if(query.first())
    {
        return query.value(0).toInt();
    }
    return 0;
}

