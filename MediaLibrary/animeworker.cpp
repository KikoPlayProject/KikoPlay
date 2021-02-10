#include "animeworker.h"
#include "globalobjects.h"
#include "Common/network.h"
#include "Common/htmlparsersax.h"
#include "Common/threadtask.h"

#include "Service/bangumi.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
/*
void AnimeWorker::addAnimeInfo(const QString &animeName,const QString &epName, const QString &path)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
    query.prepare("select Name from eps where LocalFile=?");
    query.bindValue(0,path);
    query.exec();
    if(query.first())
    {
        QString ep(query.value(0).toString());
        if(ep==epName) return;
        query.prepare("update eps set Name=? where LocalFile=?");
        query.bindValue(0,epName);
        query.bindValue(1,path);
        query.exec();
        Anime *anime=animesMap.value(animeName,nullptr);
        if(anime) anime->epList.clear();
        return;
    }
    std::function<void (const QString &,const QString &,const QString &)> insertEpInfo
            = [](const QString &animeName,const QString &epName,const QString &path)
    {
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("insert into eps(Anime,Name,LocalFile) values(?,?,?)");
        query.bindValue(0,animeName);
        query.bindValue(1,epName);
        query.bindValue(2,path);
        query.exec();
    };
    Anime *anime=animesMap.value(animeName,nullptr);
    if(anime)
    {
        insertEpInfo(animeName,epName,path);
        anime->epList.clear();
    }
    else
    {
        QString aliasOf(isAlias(animeName));
        if(aliasOf.isEmpty())
        {
            Anime *anime=new Anime;
            anime->id="";
            anime->name=animeName;
            anime->epCount=0;
            anime->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
            QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
            query.prepare("insert into anime(Anime,AddTime,BangumiID) values(?,?,?)");
            query.bindValue(0,anime->name);
            query.bindValue(1,anime->addTime);
            query.bindValue(2,anime->id);
            query.exec();
            insertEpInfo(animeName,epName,path);
            animesMap.insert(animeName,anime);
            emit addAnime(anime);
        }
        else
        {
            insertEpInfo(aliasOf,epName,path);
            Anime *anime=animesMap.value(aliasOf,nullptr);
            if(anime) //The anime has been loaded
            {
                anime->epList.clear();
            }
        }
    }

}
*/
/*
void AnimeWorker::addAnimeInfo(const QString &animeName, int bgmId)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("select * from anime where Anime=?");
    query.bindValue(0,animeName);
    query.exec();
    if(query.first()) return;
    Q_ASSERT(!animesMap.contains(animeName));
    Anime *anime=new Anime;
    anime->id= bgmId;
    anime->name=animeName;
    anime->epCount=0;
    anime->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
    query.prepare("insert into anime(Anime,AddTime,BangumiID) values(?,?,?)");
    query.bindValue(0,anime->name);
    query.bindValue(1,anime->addTime);
    query.bindValue(2,anime->id);
    query.exec();
    animesMap.insert(animeName,anime);
    emit addAnime(anime);
}
*/
/*
void AnimeWorker::downloadDetailInfo(Anime *anime, int bangumiId)
{
    QString animeUrl(QString("https://api.bgm.tv/subject/%1").arg(bangumiId));
    QUrlQuery animeQuery;
    QString errInfo;
    animeQuery.addQueryItem("responseGroup","medium");
    try
    {
        emit downloadDetailMessage(tr("Downloading General Info..."));
        QString str(Network::httpGet(animeUrl,animeQuery,QStringList()<<"Accept"<<"application/json"));
        QJsonDocument document(Network::toJson(str));
        if(!document.isObject())
        {
            throw Network::NetworkError(tr("Json Format Error"));
        }
        QJsonObject obj = document.object();
        if(obj.value("type").toInt()!=2)
        {
            throw Network::NetworkError(tr("Json Format Error"));
        }
        anime->id=bangumiId;
        QString newTitle(obj.value("name_cn").toString());
        if(newTitle.isEmpty())newTitle=obj.value("name").toString();
        newTitle.replace("&amp;","&");
        if(newTitle!=anime->name)
        {
            animesMap.remove(anime->name);
            setAlias(newTitle,anime->name);
            QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
            if(animesMap.contains(newTitle))
            {
                query.prepare("update eps set Anime=? where Anime=?");
                query.bindValue(0,newTitle);
                query.bindValue(1,anime->name);
                query.exec();
                animesMap[newTitle]->epList.clear();
                emit mergeAnime(anime,animesMap[newTitle]);
                return;
            }
            query.prepare("update anime set Anime=? where Anime=?");
            query.bindValue(0,newTitle);
            query.bindValue(1,anime->name);
            query.exec();
            anime->name=newTitle;
            animesMap.insert(newTitle,anime);
        }
        anime->desc=obj.value("summary").toString();
        anime->airDate=obj.value("air_date").toString();
        anime->epCount=obj.value("eps_count").toInt();
        int quality = GlobalObjects::appSetting->value("Library/CoverQuality", 1).toInt() % 3;
        const char *qualityTypes[] = {"images/medium","images/common","images/large"};
        QByteArray cover(Network::httpGet(Network::getValue(obj,qualityTypes[quality]).toString(),QUrlQuery()));
        anime->coverPixmap.loadFromData(cover);

        QJsonArray staffArray(obj.value("staff").toArray());
        anime->staff.clear();
        for(auto staffIter=staffArray.begin();staffIter!=staffArray.end();++staffIter)
        {
            QJsonObject staffObj=(*staffIter).toObject();
            QJsonArray jobArray(staffObj.value("jobs").toArray());
            for(auto jobIter=jobArray.begin();jobIter!=jobArray.end();++jobIter)
            {
                bool contains=false;
                QString job((*jobIter).toString());
                for(auto iter=anime->staff.begin();iter!=anime->staff.end();++iter)
                {
                    if((*iter).first==job)
                    {
                        (*iter).second+=" "+staffObj.value("name").toString();
                        contains=true;
                        break;
                    }
                }
                if(!contains)
                {
                    anime->staff.append(QPair<QString,QString>(job,staffObj.value("name").toString()));
                }
            }
        }
        anime->characters.clear();
        QJsonArray crtArray(obj.value("crt").toArray());
        emit downloadDetailMessage(tr("Downloading Character Info..."));

        for(auto crtIter=crtArray.begin();crtIter!=crtArray.end();++crtIter)
        {
            QJsonObject crtObj=(*crtIter).toObject();
            Character crt;
            crt.name=crtObj.value("name").toString();
            //crt.name_cn=crtObj.value("name_cn").toString();
            crt.id=crtObj.value("id").toInt();
            crt.actor=crtObj.value("actors").toArray().first().toObject().value("name").toString();
            QString imgUrl(Network::getValue(crtObj, "images/grid").toString());
            try
            {
                if(!imgUrl.isEmpty())
                {
                    crt.imgURL=imgUrl;
                    //crt.image=Network::httpGet(imgUrl,QUrlQuery());
                }
            } catch (Network::NetworkError &error) {
                emit downloadDetailMessage(tr("Downloading Character Info...%1 Failed: %2").arg(crt.name, error.errorInfo));
                                           //.arg(crt.name_cn.isEmpty()?crt.name:crt.name_cn, error.errorInfo));
            }
            anime->characters.append(crt);
        }
        updateAnimeInfo(anime);                      
        if(GlobalObjects::appSetting->value("Library/DownloadTag",Qt::Checked).toInt()==Qt::Checked)
        {
            emit downloadDetailMessage(tr("Downloading Label Info..."));
            downloadLabelInfo(anime);
        }
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
        emit downloadDetailMessage(tr("Downloading General Info Failed: %1").arg(errInfo));
    }
    emit downloadDone(errInfo);
}
*/
/*
void AnimeWorker::downloadTags(int bangumiID, QStringList &tags)
{
    QString errInfo(Bangumi::getTags(bangumiID, tags));
    emit downloadTagDone(errInfo);
}
*/
/*
void AnimeWorker::saveTags(const QString &title, const QStringList &tags)
{
    QSqlDatabase db=QSqlDatabase::database("Bangumi_W");
    db.transaction();
    QSqlQuery query(db);
    query.prepare("insert into tag(Anime,Tag) values(?,?)");
    query.bindValue(0,title);
    for(const QString &tag:tags)
    {
        query.bindValue(1,tag);
        query.exec();
    }
    db.commit();
}
*/

/*
void AnimeWorker::loadAnimes(QList<Anime *> *animes,int offset,int limit)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.exec(QString("select * from anime order by AddTime desc limit %1 offset %2").arg(limit).arg(offset));
    int animeNo=query.record().indexOf("Anime"),
        timeNo=query.record().indexOf("AddTime"),
        epCountNo=query.record().indexOf("EpCount"),
        summaryNo=query.record().indexOf("Summary"),
        dateNo=query.record().indexOf("Date"),
        staffNo=query.record().indexOf("Staff"),
        bangumiIdNo=query.record().indexOf("BangumiID"),
        coverNo=query.record().indexOf("Cover");
    int count=0;
    while (query.next())
    {
        Anime *anime=new Anime;
        anime->name=query.value(animeNo).toString();
        anime->desc=query.value(summaryNo).toString();
        anime->airDate=query.value(dateNo).toString();
        anime->addTime=query.value(timeNo).toLongLong();
        anime->epCount=query.value(epCountNo).toInt();
        //anime->loadCrtImage=false;
        QStringList staffs(query.value(staffNo).toString().split(';',QString::SkipEmptyParts));
        for(int i=0;i<staffs.count();++i)
        {
            int pos=staffs.at(i).indexOf(':');
            anime->staff.append(QPair<QString,QString>(staffs[i].left(pos),staffs[i].mid(pos+1)));
        }
        anime->id=query.value(bangumiIdNo).toInt();
        anime->coverPixmap.loadFromData(query.value(coverNo).toByteArray());

        QSqlQuery crtQuery(QSqlDatabase::database("Bangumi_W"));
        crtQuery.prepare("select * from character where Anime=?");
        crtQuery.bindValue(0,anime->name);
        crtQuery.exec();
        int nameNo=crtQuery.record().indexOf("Name"),
            nameCNNo=crtQuery.record().indexOf("NameCN"),
            actorNo=crtQuery.record().indexOf("Actor"),
            idNo=crtQuery.record().indexOf("BangumiID");
            //imageNo=crtQuery.record().indexOf("Image");
        while (crtQuery.next())
        {
            Character crt;
            crt.name=crtQuery.value(nameNo).toString();
            //crt.name_cn=crtQuery.value(nameCNNo).toString();
            crt.id=crtQuery.value(idNo).toInt();
            crt.actor=crtQuery.value(actorNo).toString();
            //crt.image=crtQuery.value(imageNo).toByteArray();
            anime->characters.append(crt);
        }
        animes->append(anime);
        animesMap.insert(anime->name,anime);
        count++;
    }
    emit loadDone(count);
}
*/

void AnimeWorker::deleteAnime(Anime *anime)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([=](){
        emit removeTags(anime->_name,anime->_airDate.left(7));
        QSqlDatabase db=GlobalObjects::getDB(GlobalObjects::Bangumi_DB);
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

/*
void AnimeWorker::updatePlayTime(const QString &title, const QString &path)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("update eps set LastPlayTime=? where LocalFile=?");
    QString timeStr(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    query.bindValue(0,timeStr);
    query.bindValue(1,path);
    query.exec();
    if(animesMap.contains(title))
    {
        Anime *anime=animesMap[title];
        for(auto iter=anime->epList.begin();iter!=anime->epList.end();++iter)
        {
            if((*iter).localFile==path)
            {
                (*iter).lastPlayTime=QDateTime::currentDateTime().toSecsSinceEpoch();
                break;
            }
        }

    }
}
*/

/*
void AnimeWorker::loadLabelInfo(QMap<QString, QSet<QString> > &tagMap, QMap<QString,int> &timeMap)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("select * from tag");
    query.exec();
    int animeNo=query.record().indexOf("Anime"),tagNo=query.record().indexOf("Tag");
    QString tagName;
    while (query.next())
    {
        tagName=query.value(tagNo).toString();
        if(!tagMap.contains(tagName))
            tagMap.insert(tagName,QSet<QString>());
        tagMap[tagName].insert(query.value(animeNo).toString());
    }

    query.prepare("select Date from anime");
    query.exec();
    int dateNo=query.record().indexOf("Date");
    QString dateStr;
    while (query.next())
    {
        dateStr=query.value(dateNo).toString().left(7);
        if(dateStr.isEmpty())continue;
        if(!timeMap.contains(dateStr))
        {
            timeMap.insert(dateStr,0);
        }
        timeMap[dateStr]++;
    }
    emit loadLabelInfoDone();
}
*/

/*
void AnimeWorker::updateCrtImage(const QString &title, const Character *crt)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("update character_image set Image=? where Anime=? and CBangumiID=?");
    //query.bindValue(0,crt->image);
    query.bindValue(0,QPixmap());
    query.bindValue(1,title);
    query.bindValue(2,crt->id);
    query.exec();
}
*/
/*
void AnimeWorker::deleteTag(const QString &tag, const QString &animeTitle)
{
    QSqlDatabase db=QSqlDatabase::database("Bangumi_W");
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
}
*/

AnimeWorker::AnimeWorker(QObject *parent):QObject(parent)
{
    qRegisterMetaType<EpInfo>("EpInfo");
    qRegisterMetaType<AnimeImage>("AnimeImage");
    qRegisterMetaType<EpType>("EpType");
}

AnimeWorker::~AnimeWorker()
{
    qDeleteAll(animesMap);
}

int AnimeWorker::fetchAnimes(QList<Anime *> *animes, int offset, int limit)
{
    ThreadTask task(GlobalObjects::workThread);
    return task.Run([=](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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
        QSqlQuery crtQuery(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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
            anime->_cover.loadFromData(query.value(coverNo).toByteArray());

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
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
    query.exec("select count(Anime) from anime");
    if(query.first())
    {
        return query.value(0).toInt();
    }
    return 0;
}

void AnimeWorker::loadCrImages(Anime *anime)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([anime](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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
        return 0;
    });
}

void AnimeWorker::loadEpInfo(Anime *anime)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([anime](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("select * from episode where Anime=?");
        query.bindValue(0,anime->name());
        query.exec();
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
        return 0;
    });
}

void AnimeWorker::loadPosters(Anime *anime)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([anime](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("select TimeId, Info, Thumb from image where Anime=? and Type=?");
        query.bindValue(0,anime->name());
        query.bindValue(1,int(AnimeImage::ImageType::POSTER));
        query.exec();
        int timeIdNo=query.record().indexOf("TimeId"),
            infoNo=query.record().indexOf("Info"),
            thumbNo=query.record().indexOf("Thumb");
        while (query.next())
        {
            AnimeImage poster;
            poster.type = AnimeImage::ImageType::POSTER;
            poster.timeId = query.value(timeIdNo).toLongLong();
            poster.info = query.value(infoNo).toString();
            poster.thumb.loadFromData(query.value(thumbNo).toByteArray());
            anime->posters.append(poster);
        }
        return 0;
    });
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
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("insert into anime(Anime,AddTime,ScriptId,ScriptData) values(?,?,?,?)");
        query.bindValue(0,anime->_name);
        query.bindValue(1,anime->_addTime);
        query.bindValue(2,anime->_scriptId);
        query.bindValue(3,anime->_scriptData);
        query.exec();
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
            QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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
        if(animeInMap || checkAnimeExist(anime->_name))  //anime exists, update infomation
        {
            if(animeInMap == anime) return false;
            delete anime;
            return false;
        }
        else  //anime not exits, add it to library
        {
            QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
            query.prepare("insert into anime(Anime,AddTime) values(?,?)");
            query.bindValue(0,anime->_name);
            query.bindValue(1,anime->_addTime);
            query.exec();
            updateAnimeInfo(anime);
            animesMap.insert(anime->_name,anime);
            emit animeAdded(anime);
            if(!anime->_airDate.isEmpty())
                emit addTimeLabel(anime->_airDate.left(7), "");
            return true;
        }
    }).toBool();
}

void AnimeWorker::addAnime(Anime *srcAnime, Anime *newAnime)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([=](){
        Q_ASSERT(animesMap.contains(srcAnime->_name));
        if(srcAnime->_name!=newAnime->_name)
        {
            addAlias(newAnime->_name, srcAnime->_name);
            Anime *animeInMap = animesMap.value(newAnime->_name, nullptr);
            QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
            if(animeInMap || checkAnimeExist(newAnime->_name))  //newAnime exists, only merge episodes of srcAnime to newAnime
            {
                query.prepare("update episode set Anime=? where Anime=?");
                query.bindValue(0,newAnime->_name);
                query.bindValue(1,srcAnime->_name);
                query.exec();
                if(animeInMap && animeInMap->epLoaded)
                {
                    animeInMap->epLoaded = false;
                    emit epReset(animeInMap->_name);
                }
                animesMap.remove(srcAnime->_name);
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
                        emit addTimeLabel(newAnime->_airDate.left(7), srcAnime->_airDate.left(7));
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
                    emit addTimeLabel(newAnime->_airDate.left(7), srcAnime->_airDate.left(7));
                srcAnime->assign(newAnime);
                emit animeUpdated(srcAnime);
            }
        }
        delete newAnime;
        return 0;
    });
}

void AnimeWorker::addEp(const QString &animeName, const EpInfo &ep)
{
    if(ep.localFile.isEmpty()) return;
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        if(finished)
            query.prepare("update episode set FinishTime=? where LocalFile=?");
        else
            query.prepare("update episode set LastPlayTime=? where LocalFile=?");
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
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("select Anime episode where LocalFile=?");
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

void AnimeWorker::saveCrtImage(const QString &animeName, const QString &crtName, const QByteArray &imageContent)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("update character set Image=? where Anime=? and Name=?");
        query.bindValue(0,imageContent);
        query.bindValue(1,animeName);
        query.bindValue(2,crtName);
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

        QImage &&thumb=image.scaled(AnimeImage::thumbW, AnimeImage::thumbH, Qt::AspectRatioMode::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        QByteArray thumbBytes;
        QBuffer bufferThumb(&thumbBytes);
        bufferThumb.open(QIODevice::WriteOnly);
        thumb.save(&bufferThumb, "PNG");

        QString alias = isAlias(animeName);
        QString matchAnimeName = alias.isEmpty()?animeName:alias;

        qint64 timeId = QDateTime::currentDateTime().toSecsSinceEpoch();
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("insert into image(Anime,Type,TimeId,Info,Thumb,Data) values(?,?,?,?,?,?)");
        query.bindValue(0,matchAnimeName);
        query.bindValue(1,AnimeImage::CAPTURE);
        query.bindValue(2,timeId);
        query.bindValue(3,thumbBytes);
        query.bindValue(4,imgBytes);

        AnimeImage aImage;
        aImage.type = AnimeImage::CAPTURE;
        aImage.info = info;
        aImage.timeId = timeId;
        aImage.thumb = QPixmap::fromImage(thumb);
        emit captureUpdated(matchAnimeName, aImage);
    });
}

const QPixmap AnimeWorker::getAnimeImageData(const QString &animeName, AnimeImage::ImageType type, qint64 timeId)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("delete from capture where Anime=? and Type=? and TimeId=?");
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
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("select Type, TimeId,Info,Thumb from image where Anime=? and (Type=? or Type=?) order by Time desc limit ? offset ?");
        query.bindValue(0,animeName);
        query.bindValue(1,AnimeImage::CAPTURE);
        query.bindValue(2,AnimeImage::SLICE);
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
            img.thumb.loadFromData(query.value(thumbNo).toByteArray());
            captureList.append(img);
            ++count;
        }
        return count;
    }).toInt();
}

void AnimeWorker::loadTags(QMap<QString, QSet<QString> > &tagMap, QMap<QString, int> &timeMap)
{
    ThreadTask task(GlobalObjects::workThread);
    task.Run([&](){
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("select * from tag");
        query.exec();
        int animeNo=query.record().indexOf("Anime"),tagNo=query.record().indexOf("Tag");
        QString tagName;
        while (query.next())
        {
            tagName=query.value(tagNo).toString();
            if(!tagMap.contains(tagName))
                tagMap.insert(tagName,QSet<QString>());
            tagMap[tagName].insert(query.value(animeNo).toString());
        }

        query.prepare("select AirDate from anime");
        query.exec();
        int dateNo=query.record().indexOf("AirDate");
        QString dateStr;
        while (query.next())
        {
            dateStr=query.value(dateNo).toString().left(7);
            if(dateStr.isEmpty())continue;
            if(!timeMap.contains(dateStr))
            {
                timeMap.insert(dateStr,0);
            }
            timeMap[dateStr]++;
        }
        return 0;
    });
}

void AnimeWorker::deleteTag(const QString &tag, const QString &animeTitle)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlDatabase db=GlobalObjects::getDB(GlobalObjects::Bangumi_DB);
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
        if(!tag.isEmpty() && !animeTitle.isEmpty())
            emit removeTagFrom(animeTitle,tag);
    });
}

void AnimeWorker::addTags(const QString &aniemName, const QStringList &tags)
{
    emit addTagsTo(aniemName,tags);
}

void AnimeWorker::saveTags(const QString &animeName, const QStringList &tags)
{
    ThreadTask task(GlobalObjects::workThread);
    task.RunOnce([=](){
        QSqlDatabase db=GlobalObjects::getDB(GlobalObjects::Bangumi_DB);
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
        QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
        query.prepare("select Alias, Anime from alias");
        query.exec();
        int aliasNo=query.record().indexOf("Alias"),
            animeNo=query.record().indexOf("Anime");
        while (query.next())
        {
            animeAlias.insert(query.value(animeNo).toString(), query.value(aliasNo).toString());
            aliasAnime.insert(query.value(aliasNo).toString(), query.value(animeNo).toString());
        }
        aliasLoaded = true;
    }
}

bool AnimeWorker::updateAnimeInfo(Anime *anime)
{

    QSqlDatabase db=GlobalObjects::getDB(GlobalObjects::Bangumi_DB);
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
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    anime->_cover.save(&buffer,"jpg");
    query.bindValue(8,bytes);
    query.bindValue(9,anime->_name);
    query.exec();

    query.prepare("delete from character where Anime=?");
    query.bindValue(0,anime->_name);
    query.exec();

    query.prepare("insert into character(Anime,Name,Actor,Link,ImageURL,Image) values(?,?,?,?,?)");
    query.bindValue(0,anime->_name);
    for(const auto &c : anime->characters)
    {
        query.bindValue(1, c.name);
        query.bindValue(2, c.actor);
        query.bindValue(3, c.link);
        query.bindValue(4, c.imgURL);
        QByteArray bytes;
        if(!c.imgURL.isEmpty())
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

/*
QString AnimeWorker::downloadLabelInfo(Anime *anime)
{
    QStringList tags;
    QString err(Bangumi::getTags(anime->id.toInt(), tags));
    if(err.isEmpty())
    {
        QRegExp yearRe("(19|20)\\d{2}");
        QStringList trivialTags={"TV","OVA","WEB"};
        QStringList tagList;
        for(auto &tagName : tags)
        {
            if(yearRe.indexIn(tagName)==-1 && !trivialTags.contains(tagName)
                    && !anime->name.contains(tagName) && !tagName.contains(anime->name))
                tagList.append(tagName);
            if(tagList.count()>12)
                break;
        }
        emit newTagDownloaded(anime->name, tagList);
    }
    return err;
}
*/

QString AnimeWorker::isAlias(const QString &name)
{
    loadAlias();
    auto i = aliasAnime.find(name);
    return i==aliasAnime.end()?"":i.value();
}

void AnimeWorker::addAlias(const QString &name, const QString &alias)
{
    if(aliasAnime.contains(alias)) return;
    animeAlias.insert(name, alias);
    aliasAnime.insert(alias, name);
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
    query.prepare("insert into alias(Alias,Anime) values(?,?)");
    query.bindValue(0,alias);
    query.bindValue(1,name);
    query.exec();

}

void AnimeWorker::removeAlias(const QString &name, const QString &alias)
{
    if(alias.isEmpty())
    {
        animeAlias.remove(name);
    }
    else
    {
        animeAlias.remove(name, alias);
        aliasAnime.remove(alias);
    }
}

bool AnimeWorker::checkAnimeExist(const QString &name)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
    query.prepare("select Anime from anime where Anime=?");
    query.bindValue(0,name);
    query.exec();
    return query.first();
}

bool AnimeWorker::checkEpExist(const QString &animeName, const EpInfo &ep)
{
    QSqlQuery query(GlobalObjects::getDB(GlobalObjects::Bangumi_DB));
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


