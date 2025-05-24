#include "animeworker.h"
#include "tagnode.h"
#include "globalobjects.h"
#include "Common/threadtask.h"
#include "Common/lrucache.h"
#include "Common/dbmanager.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QPainter>

namespace
{
LRUCache<QString, QSharedPointer<Anime>> singleAnimeCache{"SingleAnime", 128, true, true};
}

void AnimeWorker::deleteAnime(Anime *anime)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([=](){
        QSqlDatabase db = DBManager::instance()->getDB(DBManager::Bangumi);
        db.transaction();
        QSqlQuery query(db);
        query.prepare("delete from anime where Anime=?");
        query.bindValue(0,anime->_name);
        query.exec();
        query.prepare("delete from alias where Anime=?");
        query.bindValue(0,anime->_name);
        query.exec();
        animesMap.remove(anime->_name);
        db.commit();
        removeAlias(anime->_name);
        delete anime;
        return 0;
    });
}

Anime *AnimeWorker::getAnime(const QString &name)
{
    const QString alias = isAlias(name);
    const QString matchAnimeName = alias.isEmpty()? name : alias;
    return animesMap.value(matchAnimeName, nullptr);
}

bool AnimeWorker::hasAnime(const QString &name)
{
    if(animesMap.contains(name)) return true;
    return checkAnimeExist(name);
}

QSharedPointer<Anime> AnimeWorker::getSingleAnime(const QString &name)
{
    Anime *anime = getAnime(name);
    if (anime)
    {
        return QSharedPointer<Anime>::create(*anime);
    }
    if (singleAnimeCache.contains(name))
    {
        return singleAnimeCache.get(name);
    }
    ThreadTask task(GlobalObjects::workThread);
    task.Run([&]() -> int{
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("select * from anime where Anime=? limit 1");
        query.bindValue(0, name);
        query.exec();
        int animeNo = query.record().indexOf("Anime"),
            descNo = query.record().indexOf("Desc"),
            timeNo = query.record().indexOf("AddTime"),
            airDateNo = query.record().indexOf("AirDate"),
            epCountNo = query.record().indexOf("EpCount"),
            urlNo = query.record().indexOf("URL"),
            scriptIdNo = query.record().indexOf("ScriptId"),
            scriptDataNo  =query.record().indexOf("ScriptData"),
            staffNo = query.record().indexOf("Staff"),
            coverURLNo = query.record().indexOf("CoverURL"),
            coverNo = query.record().indexOf("Cover");
        if (query.first())
        {
            anime = new Anime;
            anime->_name = query.value(animeNo).toString();
            anime->_desc = query.value(descNo).toString();
            anime->_airDate = query.value(airDateNo).toString();
            anime->_addTime = query.value(timeNo).toLongLong();
            anime->_epCount = query.value(epCountNo).toInt();
            anime->_url = query.value(urlNo).toString();
            anime->_scriptId = query.value(scriptIdNo).toString();
            anime->_scriptData = query.value(scriptDataNo).toString();
            anime->setStaffs(query.value(staffNo).toString());
            anime->_coverURL = query.value(coverURLNo).toString();
            anime->_coverData = query.value(coverNo).toByteArray();

            QSqlQuery crtQuery(DBManager::instance()->getDB(DBManager::Bangumi));
            crtQuery.prepare("select Name, Actor, Link, ImageURL from character where Anime=?");
            crtQuery.bindValue(0,anime->_name);
            crtQuery.exec();
            int nameNo = crtQuery.record().indexOf("Name"),
                actorNo = crtQuery.record().indexOf("Actor"),
                linkNo = crtQuery.record().indexOf("Link"),
                imageURLNo = crtQuery.record().indexOf("ImageURL");
            while (crtQuery.next())
            {
                Character crt;
                crt.name=crtQuery.value(nameNo).toString();
                crt.actor=crtQuery.value(actorNo).toString();
                crt.link=crtQuery.value(linkNo).toString();
                crt.imgURL=crtQuery.value(imageURLNo).toString();
                anime->characters.append(crt);
            }
            return 0;
        }
        return -1;
    }, true);
    if (anime)
    {
        QSharedPointer<Anime> sp(anime);
        singleAnimeCache.put(name, sp);
        return sp;
    }
    return nullptr;
}

AnimeWorker::AnimeWorker(QObject *parent):QObject(parent)
{
    qRegisterMetaType<EpInfo>("EpInfo");
    qRegisterMetaType<AnimeImage>("AnimeImage");
    qRegisterMetaType<EpType>("EpType");
    qRegisterMetaType<TagNode::TagType>("TagNode::TagType");
    qRegisterMetaType<AnimeLite>("AnimeLite");
    aliasLoaded = false;
}

AnimeWorker::~AnimeWorker()
{
    qDeleteAll(animesMap);
}

int AnimeWorker::fetchAnimes(QVector<Anime *> *animes, int offset, int limit)
{
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.exec(QString("select * from anime order by AddTime desc limit %1 offset %2").arg(limit).arg(offset));
        int animeNo=query.record().indexOf("Anime"),
            descNo=query.record().indexOf("Desc"),
            timeNo=query.record().indexOf("AddTime"),
            airDateNo=query.record().indexOf("AirDate"),
            epCountNo=query.record().indexOf("EpCount"),
            urlNo=query.record().indexOf("URL"),
            scriptIdNo=query.record().indexOf("ScriptId"),
            scriptDataNo=query.record().indexOf("ScriptData"),
            staffNo=query.record().indexOf("Staff"),
            coverURLNo=query.record().indexOf("CoverURL"),
            coverNo=query.record().indexOf("Cover");
        QSqlQuery crtQuery(DBManager::instance()->getDB(DBManager::Bangumi));
        crtQuery.prepare("select Name, Actor, Link, ImageURL from character where Anime=?");
        int count=0;
        while (query.next())
        {
            Anime *anime=new Anime;
            anime->_name=query.value(animeNo).toString();
            anime->_desc=query.value(descNo).toString();
            anime->_airDate=query.value(airDateNo).toString();
            anime->_addTime=query.value(timeNo).toLongLong();
            anime->_epCount=query.value(epCountNo).toInt();
            anime->_url=query.value(urlNo).toString();
            anime->_scriptId=query.value(scriptIdNo).toString();
            anime->_scriptData=query.value(scriptDataNo).toString();
            anime->setStaffs(query.value(staffNo).toString());
            anime->_coverURL=query.value(coverURLNo).toString();
			anime->_coverData = query.value(coverNo).toByteArray();
            //anime->_cover.loadFromData(query.value(coverNo).toByteArray());

            crtQuery.bindValue(0,anime->_name);
            crtQuery.exec();
            int nameNo=crtQuery.record().indexOf("Name"),
                actorNo=crtQuery.record().indexOf("Actor"),
                linkNo=crtQuery.record().indexOf("Link"),
                imageURLNo=crtQuery.record().indexOf("ImageURL");
            while (crtQuery.next())
            {
                Character crt;
                crt.name=crtQuery.value(nameNo).toString();
                crt.actor=crtQuery.value(actorNo).toString();
                crt.link=crtQuery.value(linkNo).toString();
                crt.imgURL=crtQuery.value(imageURLNo).toString();
                anime->characters.append(crt);
            }
            animes->append(anime);
            animesMap.insert(anime->_name,anime);
            count++;
        }
        return count;
    }).toInt();
}

int AnimeWorker::animeCount()
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
    query.exec("select count(Anime) from anime");
    if(query.first())
    {
        return query.value(0).toInt();
    }
    return 0;
}

void AnimeWorker::loadCrImages(Anime *anime)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
    query.prepare("select Name, Image from character where Anime=?");
    query.bindValue(0,anime->name());
    query.exec();
    int nameNo=query.record().indexOf("Name"),
        imgNo=query.record().indexOf("Image");
    while (query.next())
    {
        for(Character &crt:anime->characters)
        {
            if(crt.name==query.value(nameNo).toString())
            {
                crt.image.loadFromData(query.value(imgNo).toByteArray());
                break;
            }
        }
    }
}

bool AnimeWorker::loadEpInfo(Anime *anime)
{
    auto loadFunc = [anime](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("select * from episode where Anime=?");
        query.bindValue(0,anime->name());
        bool ret = query.exec();
        int nameNo=query.record().indexOf("Name"),
            epIndexNo=query.record().indexOf("EpIndex"),
            typeNo=query.record().indexOf("Type"),
            localFileNo=query.record().indexOf("LocalFile"),
            finishTimeNo=query.record().indexOf("FinishTime"),
            lastPlayTimeNo=query.record().indexOf("LastPlayTime");
        while (query.next())
        {
            EpInfo ep;
            ep.name=query.value(nameNo).toString();
            ep.type=EpType(query.value(typeNo).toInt());
            ep.index=query.value(epIndexNo).toDouble();
            ep.localFile=query.value(localFileNo).toString();
            ep.finishTime=query.value(finishTimeNo).toLongLong();
            ep.lastPlayTime=query.value(lastPlayTimeNo).toLongLong();
            anime->epInfoList.append(ep);
        }
        std::sort(anime->epInfoList.begin(), anime->epInfoList.end());
        return ret;
    };

    return loadFunc();
}

void AnimeWorker::addAnime(const MatchResult &match)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QString alias = isAlias(match.name);
        QString matchAnimeName = alias.isEmpty()?match.name:alias;
        // If episode exists, update
        if(checkEpExist(matchAnimeName, match.ep)) return;
        // Or add anime & episode
        Anime *anime = animesMap.value(matchAnimeName, nullptr);
        //anime exists
        if(anime || !alias.isEmpty() || checkAnimeExist(matchAnimeName))
        {
            addEp(matchAnimeName, match.ep);
            return;
        }
        //anime not exist
        anime=new Anime;
        anime->_name=matchAnimeName;
        anime->_scriptId=match.scriptId;
        anime->_scriptData=match.scriptData;
        anime->_addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("insert into anime(Anime,AddTime,ScriptId,ScriptData) values(?,?,?,?)");
        query.bindValue(0,anime->_name);
        query.bindValue(1,anime->_addTime);
        query.bindValue(2,anime->_scriptId);
        query.bindValue(3,anime->_scriptData);
        query.exec();
        if(!anime->_scriptId.isEmpty()) emit addScriptTag(anime->_scriptId);
        addEp(matchAnimeName, match.ep);
        animesMap.insert(matchAnimeName,anime);
        emit animeAdded(anime);
    });
}

void AnimeWorker::addAnime(const QString &name)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        if(!animesMap.contains(name) && !checkAnimeExist(name))
        {
            Anime *anime=new Anime;
            anime->_name=name;
            anime->_addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
            query.prepare("insert into anime(Anime,AddTime) values(?,?)");
            query.bindValue(0,anime->_name);
            query.bindValue(1,anime->_addTime);
            query.exec();
            animesMap.insert(name,anime);
            emit animeAdded(anime);
        }
    });
}

bool AnimeWorker::addAnime(Anime *anime)
{
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([=](){
        Anime *animeInMap = animesMap.value(anime->_name, nullptr);
        if(animeInMap || checkAnimeExist(anime->_name))  //anime exists
        {
            if(animeInMap == anime) return false;
            delete anime;
            return false;
        }
        else  //anime not exits, add it to library
        {
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
            anime->_addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
            query.prepare("insert into anime(Anime,AddTime) values(?,?)");
            query.bindValue(0,anime->_name);
            query.bindValue(1,anime->_addTime);
            query.exec();
            updateAnimeInfo(anime);
            animesMap.insert(anime->_name,anime);
            emit animeAdded(anime);
            if(!anime->_airDate.isEmpty()) emit addTimeTag(anime->_airDate);
            if(!anime->_scriptId.isEmpty()) emit addScriptTag(anime->_scriptId);
            return true;
        }
    }).toBool();
}

const QString AnimeWorker::addAnime(Anime *srcAnime, Anime *newAnime)
{
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([=](){
        Q_ASSERT(animesMap.contains(srcAnime->_name));
        QString retAnimeName = srcAnime->_name;
        if(srcAnime->_name!=newAnime->_name)
        {
            addAlias(newAnime->_name, srcAnime->_name);
            retAnimeName = newAnime->_name;
            emit renameEpTag(srcAnime->_name, newAnime->_name);
            Anime *animeInMap = animesMap.value(newAnime->_name, nullptr);
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
            if(animeInMap || checkAnimeExist(newAnime->_name))  //newAnime exists, merge episodes of srcAnime to newAnime
            {
                query.prepare("update episode set Anime=? where Anime=?");
                query.bindValue(0,newAnime->_name);
                query.bindValue(1,srcAnime->_name);
                query.exec();
                query.prepare("update image set Anime=? where Anime=?");
                query.bindValue(0,newAnime->_name);
                query.bindValue(1,srcAnime->_name);
                query.exec();
                if(animeInMap && animeInMap->epLoaded)
                {
                    animeInMap->epLoaded = false;
                    emit epReset(animeInMap->_name);
                }
                animesMap.remove(srcAnime->_name);
                if(!srcAnime->_airDate.isEmpty()) emit removeTimeTag(srcAnime->_airDate);
                if(!srcAnime->_scriptId.isEmpty()) emit removeScriptTag(srcAnime->_scriptId);
                emit animeRemoved(srcAnime);
            }
            else  //newAnime not exist, rename srcAnime to newAnime
            {
                query.prepare("update anime set Anime=? where Anime=?");
                query.bindValue(0, newAnime->_name);
                query.bindValue(1, srcAnime->_name);
                bool success = query.exec();
                if(success && updateAnimeInfo(newAnime))
                {
                    if(srcAnime->_airDate!=newAnime->_airDate)
                    {
                        emit removeTimeTag(srcAnime->_airDate);
                        emit addTimeTag(newAnime->_airDate);
                    }
                    if(srcAnime->_scriptId!=newAnime->_scriptId)
                    {
                        emit removeScriptTag(srcAnime->_scriptId);
                        emit addScriptTag(newAnime->_scriptId);
                    }
                    animesMap.remove(srcAnime->_name);
                    srcAnime->_name=newAnime->_name;
                    animesMap.insert(srcAnime->_name, srcAnime);
                    srcAnime->assign(newAnime);
                    emit animeUpdated(srcAnime);
                }
            }
        }
        else  // has same name, only update info
        {
            if(updateAnimeInfo(newAnime))
            {
                if(srcAnime->_airDate!=newAnime->_airDate)
                {
                    emit removeTimeTag(srcAnime->_airDate);
                    emit addTimeTag(newAnime->_airDate);
                }
                if(srcAnime->_scriptId!=newAnime->_scriptId)
                {
                    emit removeScriptTag(srcAnime->_scriptId);
                    emit addScriptTag(newAnime->_scriptId);
                }
                srcAnime->assign(newAnime);
                emit animeUpdated(srcAnime);
            }
        }
        delete newAnime;
        return retAnimeName;
    }).toString();
}

void AnimeWorker::addEp(const QString &animeName, const EpInfo &ep)
{
    if(ep.localFile.isEmpty()) return;
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
    query.prepare("insert into episode(Anime,Name,EpIndex,Type,LocalFile) values(?,?,?,?,?)");
    query.bindValue(0,animeName);
    query.bindValue(1,ep.name);
    query.bindValue(2,ep.index);
    query.bindValue(3,(int)ep.type);
    query.bindValue(4,ep.localFile);
    query.exec();
    Anime *anime = animesMap.value(animeName, nullptr);
    if(anime) anime->addEp(ep);
    emit epAdded(animeName, ep);
}

void AnimeWorker::removeEp(const QString &animeName, const QString &path)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
    query.prepare("delete from episode where LocalFile=?");
    query.bindValue(0,path);
    query.exec();
    Anime *anime = animesMap.value(animeName, nullptr);
    if(anime) anime->removeEp(path);
    emit epRemoved(animeName, path);
}

void AnimeWorker::updateEpTime(const QString &animeName, const QString &path, bool finished, qint64 epTime)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        if(finished)
        {
            QSqlQuery checkQuery(DBManager::instance()->getDB(DBManager::Bangumi));
            checkQuery.prepare("select LastPlayTime from episode where LocalFile=?");
            checkQuery.bindValue(0, path);
            checkQuery.exec();
            if(checkQuery.first())
            {
                if(checkQuery.value(0).isNull())
                {
                    updateEpTime(animeName, path);
                }
            }
            query.prepare("update episode set FinishTime=? where LocalFile=?");
        }
        else
        {
            query.prepare("update episode set LastPlayTime=? where LocalFile=?");
        }
        qint64 time = epTime==0?QDateTime::currentDateTime().toSecsSinceEpoch():epTime;
        query.bindValue(0,time);
        query.bindValue(1,path);
        query.exec();
        Anime *anime = animesMap.value(animeName, nullptr);
        if(anime && anime->epLoaded)
        {
            anime->updateEpTime(path, time, finished);
            emit epUpdated(animeName, path);
        }
    });
}

void AnimeWorker::updateEpInfo(const QString &animeName, const QString &path, const EpInfo &nEp)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("update episode set Name=?, EpIndex=?, Type=? where LocalFile=?");
        query.bindValue(0,nEp.name);
        query.bindValue(1,nEp.index);
        query.bindValue(2,(int)nEp.type);
        query.bindValue(3,path);
        query.exec();
        Anime *anime = animesMap.value(animeName, nullptr);
        if(anime) anime->updateEpInfo(path, nEp);
        emit epUpdated(animeName, path);
    });
}

void AnimeWorker::updateEpPath(const QString &animeName, const QString &path, const QString &nPath)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        if(nPath==path) return;
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("select Anime from episode where LocalFile=?");
        query.bindValue(0, nPath);
        query.exec();
        if(query.first()) return;
        query.prepare("update episode set LocalFile=? where LocalFile=?");
        query.bindValue(0,nPath);
        query.bindValue(1,path);
        query.exec();
        Anime *anime = animesMap.value(animeName, nullptr);
        if(anime) anime->updateEpPath(path, nPath);
        emit epUpdated(animeName, path);
    });
}

void AnimeWorker::updateCaptureInfo(const QString &animeName, qint64 timeId, const QString &newInfo)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("update image set Info=? where Anime=? and TimeId=?");
        query.bindValue(0,newInfo);
        query.bindValue(1,animeName);
        query.bindValue(2,timeId);
        query.exec();
    });
}

std::function<void (QSqlQuery *)> AnimeWorker::updateAirDate(const QString &animeName, const QString &newAirDate, const QString &srcAirDate)
{
    return [=](QSqlQuery *query){
        query->prepare("update anime set AirDate=? where Anime=?");
        query->bindValue(0, newAirDate);
        query->bindValue(1, animeName);
        if(query->exec())
        {
            emit removeTimeTag(srcAirDate);
            emit addTimeTag(newAirDate);
        }
    };
}

std::function<void (QSqlQuery *)> AnimeWorker::updateEpCount(const QString &animeName, int newEpCount)
{
    return [=](QSqlQuery *query){
        query->prepare("update anime set EpCount=? where Anime=?");
        query->bindValue(0, newEpCount);
        query->bindValue(1, animeName);
        query->exec();
    };
}

std::function<void (QSqlQuery *)> AnimeWorker::updateStaffInfo(const QString &animeName, const QVector<QPair<QString, QString> > &staffs)
{
    QString staffStr = Anime::staffListToStr(staffs);
    return [=](QSqlQuery *query){
        query->prepare("update anime set Staff=? where Anime=?");
        query->bindValue(0, staffStr);
        query->bindValue(1, animeName);
        query->exec();
    };
}

std::function<void (QSqlQuery *)> AnimeWorker::updateDescription(const QString &animeName, const QString &desc)
{
    return [=](QSqlQuery *query){
        query->prepare("update anime set Desc=? where Anime=?");
        query->bindValue(0, desc);
        query->bindValue(1, animeName);
        query->exec();
    };
}

bool AnimeWorker::runQueryGroup(const QVector<std::function<void (QSqlQuery *)> > &queries)
{
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([=](){
        QSqlDatabase db = DBManager::instance()->getDB(DBManager::Bangumi);
        db.transaction();
        QSqlQuery query(db);
        for(const auto &q : queries)
        {
            q(&query);
        }
        return db.commit();
    }).toBool();
}

void AnimeWorker::updateCoverImage(const QString &animeName, const QByteArray &imageContent, const QString &coverURL)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        if(coverURL != Anime::emptyCoverURL)
        {
            query.prepare("update anime set Cover=?, CoverURL=? where Anime=?");
            query.bindValue(0,imageContent);
            query.bindValue(1,coverURL);
            query.bindValue(2,animeName);
        }
        else
        {
            query.prepare("update anime set Cover=? where Anime=?");
            query.bindValue(0,imageContent);
            query.bindValue(1,animeName);
        }
        query.exec();
    });
}

void AnimeWorker::updateCrtImage(const QString &animeName, const QString &crtName, const QByteArray &imageContent)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("update character set Image=? where Anime=? and Name=?");
        query.bindValue(0,imageContent);
        query.bindValue(1,animeName);
        query.bindValue(2,crtName);
        query.exec();
    });
}

void AnimeWorker::addCharacter(const QString &animeName, const Character &crt)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("insert into character(Anime,Name,Actor,Link) values(?,?,?,?)");
        query.bindValue(0, animeName);
        query.bindValue(1, crt.name);
        query.bindValue(2, crt.actor);
        query.bindValue(3, crt.link);
        query.exec();
    });
}

void AnimeWorker::modifyCharacter(const QString &animeName, const QString &srcCrtName, const Character &crtInfo)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("update character set Name=?, Actor=?, Link=? where Anime=? and Name=?");
        query.bindValue(0, crtInfo.name);
        query.bindValue(1, crtInfo.actor);
        query.bindValue(2, crtInfo.link);
        query.bindValue(3, animeName);
        query.bindValue(4, srcCrtName);
        query.exec();
    });
}

void AnimeWorker::removeCharacter(const QString &animeName, const QString &crtName)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("delete from character where Anime=? and Name=?");
        query.bindValue(0,animeName);
        query.bindValue(1,crtName);
        query.exec();
    });
}

void AnimeWorker::saveCapture(const QString &animeName, const QString &info, const QImage &image)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QByteArray imgBytes;
        QBuffer bufferImage(&imgBytes);
        bufferImage.open(QIODevice::WriteOnly);
        image.save(&bufferImage, "JPG");

        QImage thumbScaled=image.scaled(AnimeImage::thumbW * GlobalObjects::context()->devicePixelRatioF, AnimeImage::thumbH * GlobalObjects::context()->devicePixelRatioF, Qt::AspectRatioMode::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QByteArray thumbBytes;
        QBuffer bufferThumb(&thumbBytes);
        bufferThumb.open(QIODevice::WriteOnly);
        thumbScaled.save(&bufferThumb, "PNG");

        QString alias = isAlias(animeName);
        QString matchAnimeName = alias.isEmpty()?animeName:alias;

        qint64 timeId = QDateTime::currentDateTime().toMSecsSinceEpoch();
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("insert into image(Anime,Type,TimeId,Info,Thumb,Data) values(?,?,?,?,?,?)");
        query.bindValue(0,matchAnimeName);
        query.bindValue(1,AnimeImage::CAPTURE);
        query.bindValue(2,timeId);
        query.bindValue(3,info);
        query.bindValue(4,thumbBytes);
        query.bindValue(5,imgBytes);
        query.exec();

        AnimeImage aImage;
        aImage.type = AnimeImage::CAPTURE;
        aImage.info = info;
        aImage.timeId = timeId;
        aImage.setRoundedThumb(thumbScaled);
        emit captureUpdated(matchAnimeName, aImage);
    });
}

void AnimeWorker::saveSnippet(const QString &animeName, const QString &info, qint64 timeId, const QImage &image)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){

        QImage thumb=image.scaled(AnimeImage::thumbW * GlobalObjects::context()->devicePixelRatioF, AnimeImage::thumbH * GlobalObjects::context()->devicePixelRatioF, Qt::AspectRatioMode::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        thumb.setDevicePixelRatio(GlobalObjects::context()->devicePixelRatioF);
        QPainter painter(&thumb);
        painter.setFont(QFont(GlobalObjects::normalFont, 8));
        QSize size = painter.fontMetrics().size(Qt::TextSingleLine, tr("Snippet")) + QSize(8, 4);
        QRect snippetTipRect({0, (int)((thumb.height() / GlobalObjects::context()->devicePixelRatioF - size.height()))}, size);

        painter.fillRect(snippetTipRect, QColor(0, 0, 0, 160));
        painter.setPen(Qt::white);
        painter.drawText(snippetTipRect, Qt::AlignCenter, tr("Snippet"));

        QByteArray thumbBytes;
        QBuffer bufferThumb(&thumbBytes);
        bufferThumb.open(QIODevice::WriteOnly);
        thumb.save(&bufferThumb, "PNG");

        QString alias = isAlias(animeName);
        QString matchAnimeName = alias.isEmpty()?animeName:alias;

        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("insert into image(Anime,Type,TimeId,Info,Thumb) values(?,?,?,?,?)");
        query.bindValue(0,matchAnimeName);
        query.bindValue(1,AnimeImage::SNIPPET);
        query.bindValue(2,timeId);
        query.bindValue(3,info);
        query.bindValue(4,thumbBytes);
        query.exec();

        AnimeImage aImage;
        aImage.type = AnimeImage::SNIPPET;
        aImage.info = info;
        aImage.timeId = timeId;
        aImage.setRoundedThumb(thumb);
        emit captureUpdated(matchAnimeName, aImage);
    });
}

const QPixmap AnimeWorker::getAnimeImageData(const QString &animeName, AnimeImage::ImageType type, qint64 timeId)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
    query.prepare("select Data from image where Anime=? and Type=? and TimeId=?");
    query.bindValue(0,animeName);
    query.bindValue(1,type);
    query.bindValue(2,timeId);
    query.exec();
    QPixmap image;
    if(query.first())
    {
        image.loadFromData(query.value(0).toByteArray());
    }
    return image;
}

void AnimeWorker::deleteAnimeImage(const QString &animeName, AnimeImage::ImageType type, qint64 timeId)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("delete from image where Anime=? and Type=? and TimeId=?");
        query.bindValue(0,animeName);
        query.bindValue(1,type);
        query.bindValue(2,timeId);
        query.exec();
    });
}

int AnimeWorker::fetchCaptures(const QString &animeName, QList<AnimeImage> &captureList, int offset, int limit)
{
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([&](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("select Type, TimeId,Info,Thumb from image where Anime=? and (Type=? or Type=?) order by TimeId desc limit ? offset ?");
        query.bindValue(0,animeName);
        query.bindValue(1,AnimeImage::CAPTURE);
        query.bindValue(2,AnimeImage::SNIPPET);
        query.bindValue(3,limit);
        query.bindValue(4,offset);
        query.exec();
        int typeNo=query.record().indexOf("Type"),
            timeIdNo=query.record().indexOf("TimeId"),
            infoNo=query.record().indexOf("Info"),
            thumbNo=query.record().indexOf("Thumb");
        int count=0;
        while (query.next())
        {
            AnimeImage img;
            img.type = AnimeImage::ImageType(query.value(typeNo).toInt());
            img.timeId = query.value(timeIdNo).toLongLong();
            img.info = query.value(infoNo).toString();
            img.setRoundedThumb(QImage::fromData(query.value(thumbNo).toByteArray()));
            captureList.append(img);
            ++count;
        }
        return count;
    }).toInt();
}

void AnimeWorker::loadAnimeInfoTag(AnimeInfoTag &animeInfoTags)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([&](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("select AirDate, ScriptId from anime");
        query.exec();
        int dateNo=query.record().indexOf("AirDate"), scriptIdNo=query.record().indexOf("ScriptId");
        animeInfoTags.clear();
        while (query.next())
        {
            QString airDate(query.value(dateNo).toString().left(7));
            QString scriptId(query.value(scriptIdNo).toString());
            if(!airDate.isEmpty())
                ++animeInfoTags.airDateCount[airDate];
            if(!scriptId.isEmpty())
                ++animeInfoTags.scriptIdCount[scriptId];
        }
        return 0;
    });
}

void AnimeWorker::loadEpInfoTag(QMap<QString, QSet<QString> > &epPathAnimes)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([&](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("select Anime, LocalFile from episode");
        query.exec();
        int animeNo=query.record().indexOf("Anime"),pathNo=query.record().indexOf("LocalFile");
        epPathAnimes.clear();
        while (query.next())
        {
            QString epPath(query.value(pathNo).toString());
            QString filePath = epPath.mid(0, epPath.lastIndexOf('/'));
            epPathAnimes[filePath].insert(query.value(animeNo).toString());
        }
        return 0;
    });
}

void AnimeWorker::loadCustomTags(QMap<QString, QSet<QString> > &tagAnimes)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([&](){
        QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
        query.prepare("select * from tag");
        query.exec();
        tagAnimes.clear();
        int animeNo=query.record().indexOf("Anime"),tagNo=query.record().indexOf("Tag");
        while (query.next())
        {
            tagAnimes[query.value(tagNo).toString()].insert(query.value(animeNo).toString());
        }
        return 0;
    });
}

void AnimeWorker::deleteTag(const QString &tag, const QString &animeTitle)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlDatabase db = DBManager::instance()->getDB(DBManager::Bangumi);
        QSqlQuery query(db);
        db.transaction();
        if(animeTitle.isEmpty())
        {
            query.prepare("delete from tag where Tag=?");
            query.bindValue(0,tag);
        }
        else if(tag.isEmpty())
        {
            query.prepare("delete from tag where Anime=?");
            query.bindValue(0,animeTitle);
        }
        else
        {
            query.prepare("delete from tag where Anime=? and Tag=?");
            query.bindValue(0,animeTitle);
            query.bindValue(1,tag);
        }
        query.exec();
        db.commit();
        //if(!tag.isEmpty() && !animeTitle.isEmpty())
        //    emit removeTagFrom(animeTitle,tag);
    });
}

void AnimeWorker::deleteTags(const QStringList &tags)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlDatabase db = DBManager::instance()->getDB(DBManager::Bangumi);
        QSqlQuery query(db);
        db.transaction();
        for(const QString &tag : tags)
        {
            query.prepare("delete from tag where Tag=?");
            query.bindValue(0,tag);
            query.exec();
        }
        query.exec();
        db.commit();
    });
}

void AnimeWorker::saveTags(const QString &animeName, const QStringList &tags)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlDatabase db = DBManager::instance()->getDB(DBManager::Bangumi);
        db.transaction();
        QSqlQuery query(db);
        query.prepare("insert into tag(Anime,Tag) values(?,?)");
        query.bindValue(0,animeName);
        for(const QString &tag:tags)
        {
            query.bindValue(1,tag);
            query.exec();
        }
        db.commit();
    });
}

void AnimeWorker::loadAlias()
{
    if(!aliasLoaded)
    {
        ThreadTask task(GlobalObjects::workThread);
        task.Run([&](){
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
            query.prepare("select Alias, Anime from alias");
            if (query.exec())
            {
                int aliasNo = query.record().indexOf("Alias"),
                    animeNo = query.record().indexOf("Anime");
                while (query.next())
                {
                    animeAlias.insert(query.value(animeNo).toString(), query.value(aliasNo).toString());
                    aliasAnime.insert(query.value(aliasNo).toString(), query.value(animeNo).toString());
                }
                aliasLoaded = true;
            }
            return 0;
        });
    }
}

bool AnimeWorker::updateAnimeInfo(Anime *anime)
{

    QSqlDatabase db = DBManager::instance()->getDB(DBManager::Bangumi);
    db.transaction();

    QSqlQuery query(db);
    query.prepare("update anime set Desc=?,AirDate=?,EpCount=?,URL=?,ScriptId=?,ScriptData=?,Staff=?,CoverURL=?,Cover=? where Anime=?");
    query.bindValue(0,anime->_desc);
    query.bindValue(1,anime->_airDate);
    query.bindValue(2,anime->_epCount);
    query.bindValue(3,anime->_url);
    query.bindValue(4,anime->_scriptId);
    query.bindValue(5,anime->_scriptData);
    query.bindValue(6,anime->staffToStr());
    query.bindValue(7,anime->_coverURL);
    query.bindValue(8,anime->_coverData);
    query.bindValue(9,anime->_name);
    query.exec();

    query.prepare("delete from character where Anime=?");
    query.bindValue(0,anime->_name);
    query.exec();

    query.prepare("insert into character(Anime,Name,Actor,Link,ImageURL,Image) values(?,?,?,?,?,?)");
    query.bindValue(0,anime->_name);
    for(const auto &c : anime->characters)
    {
        query.bindValue(1, c.name);
        query.bindValue(2, c.actor);
        query.bindValue(3, c.link);
        query.bindValue(4, c.imgURL);
        QByteArray bytes;
        if(!c.image.isNull())
        {
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            c.image.save(&buffer,"jpg");
        }
        query.bindValue(5, bytes);
        query.exec();
    }
    return db.commit();
}

QString AnimeWorker::isAlias(const QString &name)
{
    loadAlias();
    auto i = aliasAnime.find(name);
    return i==aliasAnime.end()?"":i.value();
}

bool AnimeWorker::addAlias(const QString &name, const QString &alias)
{
    if(aliasAnime.contains(alias)) return false;
    animeAlias.insert(name, alias);
    aliasAnime.insert(alias, name);
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
    query.prepare("insert into alias(Alias,Anime) values(?,?)");
    query.bindValue(0,alias);
    query.bindValue(1,name);
    return query.exec();
}

void AnimeWorker::removeAlias(const QString &name, const QString &alias, bool updateDB)
{
    if(alias.isEmpty())
    {
        animeAlias.remove(name);
        if(updateDB)
        {
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
            query.prepare("delete from alias where Anime=?");
            query.bindValue(0, name);
            query.exec();
        }
    }
    else
    {
        animeAlias.remove(name, alias);
        aliasAnime.remove(alias);
        if(updateDB)
        {
            QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
            query.prepare("delete from alias where Anime=? and Alias=?");
            query.bindValue(0, name);
            query.bindValue(1, alias);
            query.exec();
        }
    }
}

const QStringList AnimeWorker::getAlias(const QString &animeName)
{
    loadAlias();
    return animeAlias.values(animeName);
}

bool AnimeWorker::checkAnimeExist(const QString &name)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
    query.prepare("select Anime from anime where Anime=?");
    query.bindValue(0,name);
    query.exec();
    return query.first();
}

bool AnimeWorker::checkEpExist(const QString &animeName, const EpInfo &ep)
{
    QSqlQuery query(DBManager::instance()->getDB(DBManager::Bangumi));
    query.prepare("select Anime, Name, EpIndex, Type from episode where LocalFile=?");
    query.bindValue(0, ep.localFile);
    query.exec();
    int animeNo=query.record().indexOf("Anime"),
        nameNo=query.record().indexOf("Name"),
        epIndexNo=query.record().indexOf("EpIndex"),
        typeNo=query.record().indexOf("EpIndex");
    if(query.first())
    {
        QString aName(query.value(animeNo).toString());
        QString epName(query.value(nameNo).toString());
        double epIndex(query.value(epIndexNo).toDouble());
        EpType epType = EpType(query.value(typeNo).toInt());
        if(animeName!=aName)  // The episode belongs to another anime, remove it
        {
            removeEp(aName, ep.localFile);
            return false;
        }
        else if(ep.index!=epIndex || ep.type!=epType || ep.name!=epName)  // Fields of the episode have been changed, update them
        {
            query.prepare("update episode set Name=?, EpIndex=?, Type=? where LocalFile=?");
            query.bindValue(0,ep.name);
            query.bindValue(1,ep.index);
            query.bindValue(2,(int)ep.type);
            query.bindValue(3,ep.localFile);
            query.exec();
            Anime *anime = animesMap.value(animeName, nullptr);
            if(anime) anime->addEp(ep);
            emit epUpdated(animeName, ep.localFile);
        }
        return true;
    }
    return false;
}


