#include "animeworker.h"
#include "globalobjects.h"
#include "Common/network.h"
#include "Common/htmlparsersax.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
void AnimeWorker::addAnimeInfo(const QString &animeName,const QString &epName, const QString &path)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("select * from eps where LocalFile=?");
    query.bindValue(0,path);
    query.exec();
    if(query.first()) return;
    std::function<void (const QString &,const QString &,const QString &)> insertEpInfo
            = [](const QString &animeName,const QString &epName,const QString &path)
    {
        QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
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
        anime->eps.clear();
    }
    else
    {
        Anime *anime=new Anime;
        anime->bangumiID=-1;
        anime->title=animeName;
        anime->epCount=0;
        anime->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
        QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
        query.prepare("insert into anime(Anime,AddTime,BangumiID) values(?,?,?)");
        query.bindValue(0,anime->title);
        query.bindValue(1,anime->addTime);
        query.bindValue(2,anime->bangumiID);
        query.exec();
        insertEpInfo(animeName,epName,path);
        animesMap.insert(animeName,anime);
        emit addAnime(anime);
    }

}

void AnimeWorker::addAnimeInfo(const QString &animeName, int bgmId)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("select * from anime where Anime=?");
    query.bindValue(0,animeName);
    query.exec();
    if(query.first()) return;
    Q_ASSERT(!animesMap.contains(animeName));
    Anime *anime=new Anime;
    anime->bangumiID=bgmId;
    anime->title=animeName;
    anime->epCount=0;
    anime->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
    query.prepare("insert into anime(Anime,AddTime,BangumiID) values(?,?,?)");
    query.bindValue(0,anime->title);
    query.bindValue(1,anime->addTime);
    query.bindValue(2,anime->bangumiID);
    query.exec();
    animesMap.insert(animeName,anime);
    emit addAnime(anime);
}

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
        anime->bangumiID=bangumiId;
        QString newTitle(obj.value("name_cn").toString());
        if(newTitle.isEmpty())newTitle=obj.value("name").toString();
        if(newTitle!=anime->title)
        {
            animesMap.remove(anime->title);
            QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
            if(animesMap.contains(newTitle))
            {
                query.prepare("update eps set Anime=? where Anime=?");
                query.bindValue(0,newTitle);
                query.bindValue(1,anime->title);
                query.exec();
                animesMap[newTitle]->eps.clear();
                emit mergeAnime(anime,animesMap[newTitle]);
                return;
            }
            query.prepare("update anime set Anime=? where Anime=?");
            query.bindValue(0,newTitle);
            query.bindValue(1,anime->title);
            query.exec();
            anime->title=newTitle;
            animesMap.insert(newTitle,anime);
        }
        anime->summary=obj.value("summary").toString();
        anime->date=obj.value("air_date").toString();
        anime->epCount=obj.value("eps_count").toInt();
        QByteArray cover(Network::httpGet(Network::getValue(obj,"images/common").toString(),QUrlQuery()));
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
            crt.name_cn=crtObj.value("name_cn").toString();
            crt.bangumiID=crtObj.value("id").toInt();
            crt.actor=crtObj.value("actors").toArray().first().toObject().value("name").toString();
            QString imgUrl(Network::getValue(crtObj, "images/grid").toString());
            try
            {
                if(!imgUrl.isEmpty())
                {
                    crt.imgURL=imgUrl;
                    crt.image=Network::httpGet(imgUrl,QUrlQuery());
                }
            } catch (Network::NetworkError &error) {
                emit downloadDetailMessage(tr("Downloading Character Info...%1 Failed: %2")
                                           .arg(crt.name_cn.isEmpty()?crt.name:crt.name_cn, error.errorInfo));
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

void AnimeWorker::downloadTags(int bangumiID, QStringList &tags)
{
    QString bgmPageUrl(QString("http://bgm.tv/subject/%1").arg(bangumiID));
    QString errInfo;
    try
    {
        QString pageContent(Network::httpGet(bgmPageUrl,QUrlQuery()));
        QRegExp re("<div class=\"subject_tag_section\">(.*)<div id=\"panelInterestWrapper\">");
        int pos=re.indexIn(pageContent);
        if(pos!=-1)
        {
            HTMLParserSax parser(re.capturedTexts().at(1));
            while(!parser.atEnd())
            {
                if(parser.currentNode()=="a" && parser.isStartNode())
                {
                    parser.readNext();
                    tags.append(parser.readContentText());
                }
                parser.readNext();
            }
        }
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadTagDone(errInfo);
}

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
        anime->title=query.value(animeNo).toString();
        anime->summary=query.value(summaryNo).toString();
        anime->date=query.value(dateNo).toString();
        anime->addTime=query.value(timeNo).toLongLong();
        anime->epCount=query.value(epCountNo).toInt();
        anime->loadCrtImage=false;
        QStringList staffs(query.value(staffNo).toString().split(';',QString::SkipEmptyParts));
        for(int i=0;i<staffs.count();++i)
        {
            int pos=staffs.at(i).indexOf(':');
            anime->staff.append(QPair<QString,QString>(staffs[i].left(pos),staffs[i].mid(pos+1)));
        }
        anime->bangumiID=query.value(bangumiIdNo).toInt();
        anime->coverPixmap.loadFromData(query.value(coverNo).toByteArray());

        QSqlQuery crtQuery(QSqlDatabase::database("Bangumi_W"));
        crtQuery.prepare("select * from character where Anime=?");
        crtQuery.bindValue(0,anime->title);
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
            crt.name_cn=crtQuery.value(nameCNNo).toString();
            crt.bangumiID=crtQuery.value(idNo).toInt();
            crt.actor=crtQuery.value(actorNo).toString();
            //crt.image=crtQuery.value(imageNo).toByteArray();
            anime->characters.append(crt);
        }
        animes->append(anime);
        animesMap.insert(anime->title,anime);
        count++;
    }
    emit loadDone(count);
}

void AnimeWorker::deleteAnime(Anime *anime)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("delete from anime where Anime=?");
    query.bindValue(0,anime->title);
    query.exec();
    animesMap.remove(anime->title);
    delete anime;
    emit deleteDone();
}

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
        for(auto iter=anime->eps.begin();iter!=anime->eps.end();++iter)
        {
            if((*iter).localFile==path)
            {
                (*iter).lastPlayTime=timeStr;
                break;
            }
        }

    }
}

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

void AnimeWorker::updateCrtImage(const QString &title, const Character *crt)
{
    QSqlQuery query(QSqlDatabase::database("Bangumi_W"));
    query.prepare("update character_image set Image=? where Anime=? and CBangumiID=?");
    query.bindValue(0,crt->image);
    query.bindValue(1,title);
    query.bindValue(2,crt->bangumiID);
    query.exec();
}

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

AnimeWorker::~AnimeWorker()
{
    qDeleteAll(animesMap);
}

void AnimeWorker::updateAnimeInfo(Anime *anime)
{

    QSqlDatabase db=QSqlDatabase::database("Bangumi_W");
    db.transaction();

    QSqlQuery query(db);
    query.prepare("update anime set Summary=?,Date=?,Staff=?,BangumiID=?,Cover=?,EpCount=? where Anime=?");
    query.bindValue(0,anime->summary);
    query.bindValue(1,anime->date);
    QStringList staffStrList;
    for(auto iter=anime->staff.cbegin();iter!=anime->staff.cend();++iter)
        staffStrList.append((*iter).first+":"+(*iter).second);
        //staffStrList.append(iter.key()+":"+iter.value());
    query.bindValue(2,staffStrList.join(';'));
    query.bindValue(3,anime->bangumiID);
    QByteArray bytes;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    anime->coverPixmap.save(&buffer,"jpg");
    query.bindValue(4,bytes);
    query.bindValue(5,anime->epCount);
    query.bindValue(6,anime->title);
    query.exec();

    query.prepare("delete from character where Anime=?");
    query.bindValue(0,anime->title);
    query.exec();

    query.prepare("insert into character(Anime,Name,NameCN,Actor,BangumiID) values(?,?,?,?,?)");
    query.bindValue(0,anime->title);

    QSqlQuery imgQuery(db);
    imgQuery.prepare("insert into character_image(Anime,CBangumiID,ImageURL,Image) values(?,?,?,?)");
    imgQuery.bindValue(0,anime->title);
    for(auto iter=anime->characters.cbegin();iter!=anime->characters.cend();++iter)
    {
        query.bindValue(1,(*iter).name);
        query.bindValue(2,(*iter).name_cn);
        query.bindValue(3,(*iter).actor);
        query.bindValue(4,(*iter).bangumiID);
        query.exec();

        imgQuery.bindValue(1,(*iter).bangumiID);
        imgQuery.bindValue(2,(*iter).imgURL);
        imgQuery.bindValue(3,(*iter).image);
        imgQuery.exec();
    }
    db.commit();
    anime->loadCrtImage=true;
}

QString AnimeWorker::downloadLabelInfo(Anime *anime)
{
    QString bgmPageUrl(QString("http://bgm.tv/subject/%1").arg(anime->bangumiID));
    QString errInfo;
    try
    {
        QString pageContent(Network::httpGet(bgmPageUrl,QUrlQuery()));
        QRegExp re("<div class=\"subject_tag_section\">(.*)<div id=\"panelInterestWrapper\">");
        int pos=re.indexIn(pageContent);
        if(pos!=-1)
        {
            HTMLParserSax parser(re.capturedTexts().at(1));
            QRegExp yearRe("(19|20)\\d{2}");
            QStringList trivialTags={"TV","OVA","WEB"};
            QStringList tagList;
            while(!parser.atEnd())
            {
                if(parser.currentNode()=="a" && parser.isStartNode())
                {
                    parser.readNext();
                    QString tagName(parser.readContentText());
                    if(yearRe.indexIn(tagName)==-1 && !trivialTags.contains(tagName)
                            && !anime->title.contains(tagName))
                        tagList.append(tagName);
                    if(tagList.count()>9)
                        break;
                }
                parser.readNext();
            }
            emit newTagDownloaded(anime->title, tagList);
        }
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    return errInfo;
}

