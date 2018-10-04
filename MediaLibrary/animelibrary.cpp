#include "animelibrary.h"
#include "globalobjects.h"
#include "Common/network.h"
#include "Common/htmlparsersax.h"
#include <QSqlQuery>
#include <QSqlRecord>
#include <QPixmap>
#include <QCollator>
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
AnimeWorker *AnimeLibrary::animeWorker=nullptr;
AnimeLibrary::AnimeLibrary(QObject *parent):QAbstractItemModel(parent),active(false),
    currentOffset(0),hasMoreAnimes(true)
{
    comparer.setNumericMode(true);
    animeWorker=new AnimeWorker();
    animeWorker->moveToThread(GlobalObjects::workThread);
    QObject::connect(animeWorker,&AnimeWorker::addAnime,this,&AnimeLibrary::addAnime);
    QObject::connect(this,&AnimeLibrary::tryAddAnime,animeWorker,&AnimeWorker::addAnimeInfo);
    QObject::connect(animeWorker,&AnimeWorker::newTagDownloaded,this,&AnimeLibrary::addTags);
    QObject::connect(animeWorker,&AnimeWorker::loadLabelInfoDone,this,&AnimeLibrary::refreshLabelInfo);
}

AnimeLibrary::~AnimeLibrary()
{
    qDeleteAll(animes);
}

void AnimeLibrary::setActive(bool a)
{
    active=a;
    if(active)
    {
        static bool initTags=false;
        if(!initTags)
        {
            QMetaObject::invokeMethod(animeWorker,[this](){
                animeWorker->loadLabelInfo(tagsMap,timeSet);
            },Qt::QueuedConnection);
            initTags=true;
            emit animeCountChanged();
        }
        if(!tmpAnimes.isEmpty())
        {
            beginInsertRows(QModelIndex(),0,tmpAnimes.count()-1);
            while(!tmpAnimes.isEmpty())
            {
                Anime *anime=tmpAnimes.takeFirst();
                animes.prepend(anime);
            }
            endInsertRows();
            emit animeCountChanged();
        }
    }
}

void AnimeLibrary::addToLibrary(const QString &animeName, const QString &epName, const QString &path)
{
    emit tryAddAnime(animeName,epName,path);
}

Anime *AnimeLibrary::getAnime(const QModelIndex &index, bool fillInfo)
{
    if(!index.isValid()) return nullptr;
    Anime *anime=animes.at(index.row());
    if(!fillInfo)return anime;
    if(anime->eps.count()==0)
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
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
    if(anime->characters.count()==0)
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.prepare("select * from character where Anime=?");
        query.bindValue(0,anime->title);
        query.exec();
        int nameNo=query.record().indexOf("Name"),
            nameCNNo=query.record().indexOf("NameCN"),
            actorNo=query.record().indexOf("Actor"),
            idNo=query.record().indexOf("BangumiID"),
            imageNo=query.record().indexOf("Image");
        while (query.next())
        {
            Character crt;
            crt.name=query.value(nameNo).toString();
            crt.name_cn=query.value(nameCNNo).toString();
            crt.bangumiID=query.value(idNo).toInt();
            crt.actor=query.value(actorNo).toString();
            crt.image=query.value(imageNo).toByteArray();
            anime->characters.append(crt);
        }
    }
    return anime;
}

Anime *AnimeLibrary::downloadDetailInfo(Anime *anime, int bangumiId, QString *errorInfo)
{
    QString errInfo; 
    QEventLoop eventLoop;
    Anime *resultAnime =anime;
    QObject::connect(animeWorker,&AnimeWorker::downloadDone, &eventLoop,&QEventLoop::quit);
    QObject::connect(animeWorker,&AnimeWorker::mergeAnime, &eventLoop, [this,&eventLoop,&resultAnime](Anime *oldAnime,Anime *newAnime){
        QModelIndex deleteIndex=createIndex(animes.indexOf(oldAnime),0);
        deleteAnime(QModelIndexList()<<deleteIndex);
        resultAnime=newAnime;
        eventLoop.quit();
    });
    QMetaObject::invokeMethod(animeWorker,[&errInfo,anime,bangumiId,this](){
        errInfo=animeWorker->downloadDetailInfo(anime,bangumiId,tagsMap);
    },Qt::QueuedConnection);
    eventLoop.exec();
    QString animeDate(anime->date.left(4));
    if(!timeSet.contains(animeDate))
    {
        timeSet.insert(animeDate);
        emit addTimeLabel(animeDate);
    }
    *errorInfo=errInfo;
    return resultAnime;
}

void AnimeLibrary::deleteAnime(QModelIndexList &deleteIndexes)
{
    std::sort(deleteIndexes.begin(),deleteIndexes.end(),[](const QModelIndex &index1,const QModelIndex &index2){
        return index1.row()>index2.row();
    });
    foreach (const QModelIndex &index, deleteIndexes)
    {
        if(!index.isValid())return;
        Anime *anime=animes.at(index.row());
        beginRemoveRows(QModelIndex(), index.row(), index.row());
        animes.removeAt(index.row());
        endRemoveRows();
        bool last=(index==deleteIndexes.last());
        currentOffset--;
        QMetaObject::invokeMethod(animeWorker,[anime,this,last](){
            animeWorker->deleteAnime(anime);
            if(last)emit animeCountChanged();
        },Qt::QueuedConnection);
    }
}

void AnimeLibrary::deleteAnime(const QModelIndex &index)
{
    if(!index.isValid())return;
    Anime *anime=animes.at(index.row());
    beginRemoveRows(QModelIndex(), index.row(), index.row());
    animes.removeAt(index.row());
    endRemoveRows();
    currentOffset--;
    QMetaObject::invokeMethod(animeWorker,[anime,this](){
        animeWorker->deleteAnime(anime);
        emit animeCountChanged();
    },Qt::QueuedConnection);
}

void AnimeLibrary::refreshEpPlayTime(const QString &title, const QString &path)
{
    QMetaObject::invokeMethod(animeWorker,[&title,&path](){
        animeWorker->updatePlayTime(title,path);
    },Qt::QueuedConnection);
}

void AnimeLibrary::addEp(Anime *anime, const QString &epName, const QString &path)
{
    for(auto iter=anime->eps.begin();iter!=anime->eps.end();++iter)
    {
        if((*iter).localFile==path) return;
    }
    if(anime->eps.count()>0)
    {
        Episode ep;
        ep.name=epName;
        ep.localFile=path;
        anime->eps.append(ep);
        std::sort(anime->eps.begin(), anime->eps.end(), epCompare);
    }
    else
    {
        QSqlQuery query(QSqlDatabase::database("MT"));
        query.prepare("select * from eps where LocalFile=?");
        query.bindValue(0,path);
        query.exec();
        if(query.first()) return;
    }
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare("insert into eps(Anime,Name,LocalFile) values(?,?,?,?)");
    query.bindValue(0,anime->title);
    query.bindValue(1,epName);
    query.bindValue(2,path);
    query.exec();
}

void AnimeLibrary::modifyEpPath(Episode &ep, const QString &newPath)
{
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare("update eps set LocalFile=? where LocalFile=?");
    query.bindValue(0,newPath);
    query.bindValue(1,ep.localFile);
    query.exec();
    ep.localFile=newPath;
}

void AnimeLibrary::removeEp(Anime *anime,const QString &path)
{
    for(auto iter=anime->eps.begin();iter!=anime->eps.end();)
    {
        if((*iter).localFile==path) iter=anime->eps.erase(iter);
        else iter++;
    }
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare("delete from eps where LocalFile=?");
    query.bindValue(0,path);
    query.exec();
}

int AnimeLibrary::getCount(int type)
{
    QString sql;
    switch (type)
    {
    case 0://total
        sql="select count(*) from anime";
        break;
    case 1://last 3 months
    case 2://last 6 months
    case 3://last year
    {
        int timeRange[]={0,-3,-6,-12};
        qint64 secs=QDateTime::currentDateTime().addMonths(timeRange[type]).toSecsSinceEpoch();
        sql=QString("select count(*) from anime where AddTime>%1").arg(secs);
        break;
    }
    default:
        return 0;
    }
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.exec(sql);
    if(query.first())
    {
        return query.value(0).toInt();
    }
    return 0;
}

void AnimeLibrary::deleteTag(const QString &tag, const QString &animeTitle)
{
    QMetaObject::invokeMethod(animeWorker,"deleteTag",Qt::QueuedConnection,Q_ARG(QString,tag),Q_ARG(QString,animeTitle));
    if(animeTitle.isEmpty())
    {
        tagsMap.remove(tag);
    }
    else if(tagsMap.contains(tag))
    {
        tagsMap[tag].remove(animeTitle);
    }
}

void AnimeLibrary::addTag(Anime *anime, const QString &tag)
{
    if(!tagsMap.contains(tag))
    {
        tagsMap.insert(tag,QSet<QString>());
        emit addTags(QStringList()<<tag);
    }
    tagsMap[tag].insert(anime->title);
    QSqlQuery query(QSqlDatabase::database("MT"));
    query.prepare("insert into tag(Anime,Tag) values(?,?)");
    query.bindValue(0,anime->title);
    query.bindValue(1,tag);
    query.exec();
}

void AnimeLibrary::addAnime(Anime *anime)
{
    if(!active)
    {
        tmpAnimes.append(anime);
    }
    else
    {
        beginInsertRows(QModelIndex(),0,0);
        animes.prepend(anime);
        endInsertRows();
        emit animeCountChanged();
    }
    currentOffset++;
}

QVariant AnimeLibrary::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    const Anime *anime = animes.at(index.row());
	switch (role)
	{
		case Qt::DisplayRole:
		{
			return anime->title;
		}
        case AnimeRole:
        {
            return QVariant::fromValue((void *)anime);
        }
	}
    return QVariant();
}

void AnimeLibrary::fetchMore(const QModelIndex &)
{
    QList<Anime *> moreAnimes;
    QEventLoop eventLoop;
    QObject::connect(animeWorker,&AnimeWorker::loadDone, &eventLoop,&QEventLoop::quit);
    hasMoreAnimes=false;
    QMetaObject::invokeMethod(animeWorker,[this,&moreAnimes](){
        animeWorker->loadAnimes(&moreAnimes,currentOffset,limitCount);
    },Qt::QueuedConnection);
    eventLoop.exec();
    if(moreAnimes.count()>0)
    {
        hasMoreAnimes=true;
        beginInsertRows(QModelIndex(),animes.count(),animes.count()+moreAnimes.count()-1);
        animes.append(moreAnimes);
        endInsertRows();
        currentOffset+=moreAnimes.count();
    }
}

void AnimeWorker::updateAnimeInfo(Anime *anime)
{
    QSqlDatabase db=QSqlDatabase::database("WT");
    db.transaction();

    QSqlQuery query(QSqlDatabase::database("WT"));
    query.prepare("update anime set Summary=?,Date=?,Staff=?,BangumiID=?,Cover=?,EpCount=? where Anime=?");
    query.bindValue(0,anime->summary);
    query.bindValue(1,anime->date);
    QStringList staffStrList;
    for(auto iter=anime->staff.cbegin();iter!=anime->staff.cend();++iter)
        staffStrList.append((*iter).first+":"+(*iter).second);
        //staffStrList.append(iter.key()+":"+iter.value());
    query.bindValue(2,staffStrList.join(';'));
    query.bindValue(3,anime->bangumiID);
    query.bindValue(4,anime->cover);
    query.bindValue(5,anime->epCount);
    query.bindValue(6,anime->title);
    query.exec();

    query.prepare("delete from character where Anime=?");
    query.bindValue(0,anime->title);
    query.exec();

    query.prepare("insert into character(Anime,Name,NameCN,Actor,BangumiID,Image) values(?,?,?,?,?,?)");
    for(auto iter=anime->characters.cbegin();iter!=anime->characters.cend();++iter)
    {
        query.bindValue(0,anime->title);
        query.bindValue(1,(*iter).name);
        query.bindValue(2,(*iter).name_cn);
        query.bindValue(3,(*iter).actor);
        query.bindValue(4,(*iter).bangumiID);
        query.bindValue(5,(*iter).image);
        query.exec();
    }
    db.commit();
}

void AnimeWorker::addAnimeInfo(const QString &animeName,const QString &epName, const QString &path)
{
    QSqlQuery query(QSqlDatabase::database("WT"));
    query.prepare("select * from eps where LocalFile=?");
    query.bindValue(0,path);
    query.exec();
    if(query.first()) return;
    std::function<void (const QString &,const QString &,const QString &)> insertEpInfo
            = [](const QString &animeName,const QString &epName,const QString &path)
    {
        QSqlQuery query(QSqlDatabase::database("WT"));
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
        QSqlQuery query(QSqlDatabase::database("WT"));
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

QString AnimeWorker::downloadDetailInfo(Anime *anime, int bangumiId, QMap<QString, QSet<QString> > &tagMap)
{
    QString animeUrl(QString("https://api.bgm.tv/subject/%1").arg(bangumiId));
    QUrlQuery animeQuery;
    QString errInfo;
    animeQuery.addQueryItem("responseGroup","medium");
    try
    {
        QString str(Network::httpGet(animeUrl,animeQuery,QStringList()<<"Accept"<<"application/json"));
        QJsonDocument document(Network::toJson(str));
        if(!document.isObject()) return QString(tr("Json Format Error"));
        QJsonObject obj = document.object();
        if(obj.value("type").toInt()!=2)return QString(tr("Json Format Error"));
        anime->bangumiID=bangumiId;
        QString newTitle(obj.value("name_cn").toString());
        if(newTitle.isEmpty())newTitle=obj.value("name").toString();
        if(newTitle!=anime->title)
        {
            animesMap.remove(anime->title);
            QSqlQuery query(QSqlDatabase::database("WT"));
            if(animesMap.contains(newTitle))
            {
                query.prepare("update eps set Anime=? where Anime=?");
                query.bindValue(0,newTitle);
                query.bindValue(1,anime->title);
                query.exec();
                animesMap[newTitle]->eps.clear();
                emit mergeAnime(anime,animesMap[newTitle]);
                return errInfo;
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
        anime->cover=Network::httpGet(Network::getValue(obj,"images/large").toString(),QUrlQuery());
        anime->coverPixmap.loadFromData(anime->cover);

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
        for(auto crtIter=crtArray.begin();crtIter!=crtArray.end();++crtIter)
        {
            QJsonObject crtObj=(*crtIter).toObject();
            Character crt;
            crt.name=crtObj.value("name").toString();
            crt.name_cn=crtObj.value("name_cn").toString();
            crt.bangumiID=crtObj.value("id").toInt();
            crt.actor=crtObj.value("actors").toArray().first().toObject().value("name").toString();
			QString imgUrl(Network::getValue(crtObj, "images/grid").toString());
			if(!imgUrl.isEmpty())
				crt.image=Network::httpGet(imgUrl,QUrlQuery());
            anime->characters.append(crt);
        }

        updateAnimeInfo(anime);
        if(GlobalObjects::appSetting->value("Library/DownloadTag",Qt::Checked).toInt()==Qt::Checked)
            errInfo=downloadLabelInfo(anime,tagMap);
        emit downloadDone();
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    emit downloadDone();
    return errInfo;
}

void AnimeWorker::loadAnimes(QList<Anime *> *animes,int offset,int limit)
{
    QSqlQuery query(QSqlDatabase::database("WT"));
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
        QStringList staffs(query.value(staffNo).toString().split(';',QString::SkipEmptyParts));
        for(int i=0;i<staffs.count();++i)
        {
            int pos=staffs.at(i).indexOf(':');
            anime->staff.append(QPair<QString,QString>(staffs[i].left(pos),staffs[i].mid(pos+1)));
        }
        anime->bangumiID=query.value(bangumiIdNo).toInt();
        anime->cover=query.value(coverNo).toByteArray();
        anime->coverPixmap.loadFromData(anime->cover);

        QSqlQuery crtQuery(QSqlDatabase::database("WT"));
        crtQuery.prepare("select * from character where Anime=?");
        crtQuery.bindValue(0,anime->title);
        crtQuery.exec();
        int nameNo=crtQuery.record().indexOf("Name"),
            nameCNNo=crtQuery.record().indexOf("NameCN"),
            actorNo=crtQuery.record().indexOf("Actor"),
            idNo=crtQuery.record().indexOf("BangumiID"),
            imageNo=crtQuery.record().indexOf("Image");
        while (crtQuery.next())
        {
            Character crt;
            crt.name=crtQuery.value(nameNo).toString();
            crt.name_cn=crtQuery.value(nameCNNo).toString();
            crt.bangumiID=crtQuery.value(idNo).toInt();
            crt.actor=crtQuery.value(actorNo).toString();
            crt.image=crtQuery.value(imageNo).toByteArray();
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
    QSqlQuery query(QSqlDatabase::database("WT"));
    query.prepare("delete from anime where Anime=?");
    query.bindValue(0,anime->title);
    query.exec();
    animesMap.remove(anime->title);
    delete anime;
}

void AnimeWorker::updatePlayTime(const QString &title, const QString &path)
{
    QSqlQuery query(QSqlDatabase::database("WT"));
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

void AnimeWorker::loadLabelInfo(QMap<QString, QSet<QString> > &tagMap, QSet<QString> &timeSet)
{
    QSqlQuery query(QSqlDatabase::database("WT"));
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
        dateStr=query.value(dateNo).toString().left(4);
        timeSet.insert(dateStr);
    }
    emit loadLabelInfoDone();
}

void AnimeWorker::deleteTag(const QString &tag, const QString &animeTitle)
{
    QSqlQuery query(QSqlDatabase::database("WT"));
    if(animeTitle.isEmpty())
    {
        query.prepare("delete from tag where Tag=?");
        query.bindValue(0,tag);
    }
    else
    {
        query.prepare("delete from tag where Anime=? and Tag=?");
        query.bindValue(0,animeTitle);
        query.bindValue(1,tag);
    }
    query.exec();
}

QString AnimeWorker::downloadLabelInfo(Anime *anime, QMap<QString, QSet<QString> > &tagMap)
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

            QSqlDatabase db=QSqlDatabase::database("WT");
            db.transaction();
            QSqlQuery query(QSqlDatabase::database("WT"));
            query.prepare("insert into tag(Anime,Tag) values(?,?)");
            query.bindValue(0,anime->title);

            QStringList newTags;
            for(const QString &tag:tagList)
            {
                if(!tagMap.contains(tag))
                {
                    newTags.append(tag);
                    tagMap.insert(tag,QSet<QString>());
                }
                if(tagMap[tag].contains(anime->title))continue;
                tagMap[tag].insert(anime->title);
                query.bindValue(1,tag);
                query.exec();
            }
            db.commit();
            emit newTagDownloaded(newTags);
        }
    }
    catch(Network::NetworkError &error)
    {
        errInfo=error.errorInfo;
    }
    return errInfo;
}

//void AnimeFilterProxyModel::setTimeRange(int index)
//{
//    if(index==0)
//    {
//        timeAfter=0;
//        return;
//    }
//    int timeRange[]={0,-3,-6,-12};
//    timeAfter=QDateTime::currentDateTime().addMonths(timeRange[index]).toSecsSinceEpoch();
//}

bool AnimeFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
     QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
     Anime *anime=static_cast<AnimeLibrary *>(sourceModel())->getAnime(index,false);
     if(!timeFilterSet.isEmpty() && !timeFilterSet.contains(anime->date.left(4)))return false;
     for(const QString &tag:tagFilterSet)
     {
         if(!GlobalObjects::library->animeTags()[tag].contains(anime->title))
             return false;
     }
     switch (filterType)
     {
     case 0://title
         return anime->title.contains(filterRegExp());
     case 1://summary
         return anime->summary.contains(filterRegExp());
     case 2://staff
     {
         for(auto iter=anime->staff.cbegin();iter!=anime->staff.cend();++iter)
         {
             if((*iter).second.contains(filterRegExp()))
                 return true;
         }
         return false;
     }
     case 3://crt
     {
         for(auto iter=anime->characters.cbegin();iter!=anime->characters.cend();++iter)
         {
             if((*iter).name.contains(filterRegExp()) || (*iter).name_cn.contains(filterRegExp())
                     || (*iter).actor.contains(filterRegExp()))
                 return true;
         }
         return false;
     }
     }
     return true;
}
