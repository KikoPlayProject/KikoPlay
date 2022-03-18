#include "playlistprivate.h"
#include <QFile>
#include <QFileInfo>
#include <QXmlStreamReader>
#include <QCoreApplication>
#include <QRandomGenerator>

#include "globalobjects.h"
#include "Common/logger.h"
#include "Play/Video/mpvplayer.h"
#include "Play/Danmu/Manager/danmumanager.h"

PlayListPrivate::PlayListPrivate(PlayList *pl) : root(new PlayListItem), currentItem(nullptr), playListChanged(false),
    needRefresh(true), loopMode(PlayList::NO_Loop_All), autoMatch(true), modifyCounter(0), saveFinishTimeOnce(true), q_ptr(pl)
{
    PlayListItem::playlist = pl;
    plPath = GlobalObjects::dataPath + "playlist.xml";
    rectPath = GlobalObjects::dataPath + "recent.xml";
}

PlayListPrivate::~PlayListPrivate()
{
    delete root;
}

void PlayListPrivate::loadPlaylist()
{
    if(QFile::exists(plPath+".tmp") && !QFile::exists(plPath))
    {
        QFile::rename(plPath+".tmp", plPath);
    }
    QFile playlistFile(plPath);
    bool ret=playlistFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamReader reader(&playlistFile);
    QVector<PlayListItem *> parents;
    QHash<QString,int> nodeNameHash = {
        {"playlist",0},
        {"collection",1},
        {"item",2}
    };
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            switch (nodeNameHash.value(reader.name().toString()))
            {
            case 0:
            {
                parents.push_back(root);
                break;
            }
            case 1:
            {
                PlayListItem *collection=new PlayListItem(parents.last(),false);
                collection->title=reader.attributes().value("title").toString();
                collection->isBgmCollection=(reader.attributes().value("bgmCollection")=="true");
                if(collection->isBgmCollection) bgmCollectionItems.insert(collection->title, collection);
                collection->folderPath=reader.attributes().value("folderPath").toString();
                int marker=PlayListItem::Marker::M_NONE;
                if(reader.attributes().hasAttribute("marker"))
                    marker = reader.attributes().value("marker").toInt();
                if(reader.attributes().hasAttribute("addTime"))
                    collection->addTime = reader.attributes().value("addTime").toLongLong();
                collection->marker = PlayListItem::Marker(marker);
                parents.push_back(collection);
                break;
            }
            case 2:
            {
                QString title= reader.attributes().value("title").toString();
                QString animeTitle= reader.attributes().value("animeTitle").toString();
                QString poolID=reader.attributes().value("poolID").toString();
                int playTime=reader.attributes().value("playTime").toInt();
                int playTimeState=reader.attributes().value("playTimeState").toInt();
                int marker=PlayListItem::Marker::M_NONE;
                if(reader.attributes().hasAttribute("marker"))
                    marker = reader.attributes().value("marker").toInt();
                qint64 addTime = 0;
                if(reader.attributes().hasAttribute("addTime"))
                    addTime = reader.attributes().value("addTime").toLongLong();
                QString path = reader.readElementText().trimmed();

                PlayListItem *item=new PlayListItem(parents.last(),true);
                item->title=title;
                item->path= path;
                item->playTime=playTime;
                item->poolID = poolID;
                item->playTimeState=PlayListItem::PlayState(playTimeState);
                item->marker = PlayListItem::Marker(marker);
                item->addTime = addTime;
                fileItems.insert(item->path,item);
                if(!animeTitle.isEmpty())item->animeTitle=animeTitle;
                for(auto &pair :recentList)
                {
                    if(pair.first==item->path)
                    {
                        pair.second=item->animeTitle.isEmpty()?item->title:QString("%1 %2").arg(item->animeTitle, item->title);
                        break;
                    }
                }
                break;
            }
            }

        }
        if(reader.isEndElement())
        {
            int type = nodeNameHash.value(reader.name().toString());
            if(type==0)
                break;
            else if(type==1)
                parents.pop_back();
        }
        reader.readNext();
    }
    if(parents.size() > 1 || reader.hasError())
    {
        Logger::logger()->log(Logger::APP, QString("Playlist File is corrupted: %1").arg(reader.errorString()));
    }
    for(auto iter= recentList.begin();iter!=recentList.end();)
    {
        if((*iter).second.isEmpty()) //not included in playlist
            iter=recentList.erase(iter);
        else
            iter++;
    }
}

void PlayListPrivate::savePlaylist()
{
    if(!playListChanged)return;
    QFile playlistFile(plPath+".tmp");
    bool ret=playlistFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&playlistFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    saveItem(writer, root);
    writer.writeEndDocument();
    ret = QFile::remove(plPath);
    if(!QFile::exists(plPath) || ret)
    {
        ret = playlistFile.rename(plPath);
        if(ret) playListChanged=false;
    }
}

void PlayListPrivate::incModifyCounter()
{
    const int maxModifyCount = 5;
    if(++modifyCounter>=maxModifyCount)
    {
        savePlaylist();
        modifyCounter=0;
    }
}

void PlayListPrivate::saveItem(QXmlStreamWriter &writer, PlayListItem *item)
{
    if(item==root)
    {
        writer.writeStartElement("playlist");

    }
    else
    {
        writer.writeStartElement("collection");
        writer.writeAttribute("title",item->title);
        writer.writeAttribute("bgmCollection",item->isBgmCollection?"true":"false");
        if(!item->folderPath.isEmpty())
            writer.writeAttribute("folderPath",item->folderPath);
        if(item->marker != PlayListItem::Marker::M_NONE)
            writer.writeAttribute("marker", QString::number((int)item->marker));
        if(item->addTime > 0)
            writer.writeAttribute("addTime", QString::number(item->addTime));
    }
    for(PlayListItem *child : *item->children)
    {
        if(child->children)
        {
            saveItem(writer,child);
        }
        else
        {
            writer.writeStartElement("item");
            writer.writeAttribute("title", child->title);
            if(!child->animeTitle.isEmpty())
                writer.writeAttribute("animeTitle",child->animeTitle);
            if(!child->poolID.isEmpty())
                writer.writeAttribute("poolID",child->poolID);
            writer.writeAttribute("playTime",QString::number(child->playTime));
            writer.writeAttribute("playTimeState",QString::number((int)child->playTimeState));
            if(child->marker != PlayListItem::Marker::M_NONE)
                writer.writeAttribute("marker", QString::number((int)child->marker));
            if(item->addTime > 0)
                writer.writeAttribute("addTime", QString::number(item->addTime));
            writer.writeCharacters(child->path);
            writer.writeEndElement();
        }
    }
    writer.writeEndElement();
}

void PlayListPrivate::loadRecentlist()
{
    QFile recentlistFile(rectPath);
    bool ret=recentlistFile.open(QIODevice::ReadOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamReader reader(&recentlistFile);
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name=="item")
            {
                recentList.append(QPair<QString,QString>(reader.readElementText().trimmed(),QString()));
            }
        }
        reader.readNext();
    }
}

void PlayListPrivate::saveRecentlist()
{
    QFile recentlistFile(rectPath);
    bool ret=recentlistFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&recentlistFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("recentlist");
    for(auto &pair :recentList)
    {
        writer.writeStartElement("item");
        writer.writeCharacters(pair.first);
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndDocument();
}

void PlayListPrivate::updateRecentlist(PlayListItem *item)
{
    Q_Q(PlayList);
    if(!item) return;
    for(auto iter= recentList.begin();iter!=recentList.end();)
    {
        if((*iter).first==item->path)
            iter=recentList.erase(iter);
        else
            iter++;
    }
    recentList.push_front(QPair<QString,QString>(item->path,item->animeTitle.isEmpty()?item->title:QString("%1 %2").arg(item->animeTitle, item->title)));
    if(recentList.count()>q->maxRecentItems) recentList.pop_back();
    emit q->recentItemsUpdated();
}

PlayListItem *PlayListPrivate::getPrevOrNextItem(bool prev)
{
    if(!currentItem)return nullptr;
    switch (loopMode)
    {
    case PlayList::NO_Loop_All:
    case PlayList::Loop_All:
    case PlayList::Loop_One:
    case PlayList::NO_Loop_One:
    {
        PlayListItem *cur=currentItem;
        bool findAgain=false;
        int loopCounter=0;
        do
        {
            while (cur != root)
            {
                int row=cur->parent->children->indexOf(cur);
                int pos=prev?0:cur->parent->children->count()-1;
                if(row==pos)
                    cur=cur->parent;
                else
                {
                    PlayListItem *next=cur->parent->children->at(prev?row-1:row+1);
                    while(next->children && next->children->count()>0)
                        next=prev?next->children->last():next->children->first();
                    if(next->children)
                        cur=next;
                    else
                        return next;
                }
            }
            if(loopMode==PlayList::Loop_All)
            {
                if(loopCounter>0)break;
                findAgain=true;
                loopCounter++;
                while(cur->children && cur->children->count()>0)
                    cur=prev?cur->children->last():cur->children->first();
                if (!cur->children)return cur;
            }
        } while(findAgain);
        break;
    }
    case PlayList::Random:
    {
        QList<PlayListItem *> collectionItems,mediaItems;
        collectionItems.push_back(root);
        while(!collectionItems.empty())
        {
            PlayListItem *currentItem=collectionItems.front();
            collectionItems.pop_front();
            for(PlayListItem *child:*currentItem->children)
            {
                if(child->children)
                    collectionItems.push_back(child);
                else if(child!=currentItem)
                    mediaItems.push_back(child);
            }
        }
        if(mediaItems.count()>0)
        {
            int i=QRandomGenerator::global()->bounded(mediaItems.count());
            return mediaItems[i];
        }
        break;
    }
    }
    return nullptr;
}

bool PlayListPrivate::addSubFolder(QString folderStr, PlayListItem *parent, int &itemCount)
{
    QDir folder(folderStr);
    bool containsVideoFile = false;
    PlayListItem *folderCollection = new PlayListItem();
    folderCollection->title = folder.dirName();
    folderCollection->folderPath = folderStr;
    folderCollection->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
    for (QFileInfo fileInfo : folder.entryInfoList())
    {
        QString fileName = fileInfo.fileName();
        if (fileInfo.isFile())
        {
            if (GlobalObjects::mpvplayer->videoFileFormats.contains("*."+fileInfo.suffix().toLower()))
            {
                if(!fileItems.contains(fileInfo.filePath()))
                {
                    PlayListItem *newItem = new PlayListItem(folderCollection, true);
                    int suffixPos = fileName.lastIndexOf('.'), pathPos = fileName.lastIndexOf('/') + 1;
                    newItem->title = fileName.mid(pathPos, suffixPos - pathPos);
                    newItem->path = fileInfo.filePath();
                    newItem->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
                    containsVideoFile = true;
                    itemCount++;
                    fileItems.insert(newItem->path,newItem);
                }
            }
        }
        else
        {
            if (fileName == "." || fileName == "..")continue;
            bool ret = addSubFolder(fileInfo.absoluteFilePath(), folderCollection,itemCount);
            if (ret) containsVideoFile = true;
        }
    }
    if (containsVideoFile)
        folderCollection->moveTo(parent);
    else
        delete folderCollection;
    return containsVideoFile;
}

int PlayListPrivate::refreshFolder(PlayListItem *folderItem, QList<PlayListItem *> &nItems)
{
    int nCount = 0;
    QSet<QString> currentPaths;
    for(PlayListItem *item : *folderItem->children)
    {
        if(item->children)
        {
            if(!item->folderPath.isEmpty()) currentPaths<<item->folderPath;
            nCount += refreshFolder(item, nItems);
        }
    }
    if(folderItem->folderPath.isEmpty()) return nCount;

    QDir folder(folderItem->folderPath);
    bool oFolder(folderItem->parent);
    QModelIndex fIndex;
    if(oFolder) fIndex = q_ptr->createIndex(folderItem->parent->children->indexOf(folderItem),0,folderItem);
    for (QFileInfo fileInfo : folder.entryInfoList())
    {
        QString fileName(fileInfo.fileName()), filePath(fileInfo.filePath());
        if (fileInfo.isFile())
        {
            if(GlobalObjects::mpvplayer->videoFileFormats.contains("*."+fileInfo.suffix().toLower())
                    && !fileItems.contains(filePath))
            {
                if(oFolder) q_ptr->beginInsertRows(fIndex, folderItem->children->size(), folderItem->children->size());
                PlayListItem *newItem = new PlayListItem(folderItem, true);
                int suffixPos = fileName.lastIndexOf('.'), pathPos = fileName.lastIndexOf('/') + 1;
                newItem->title = fileName.mid(pathPos, suffixPos - pathPos);
                newItem->path = filePath;
                newItem->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
                fileItems.insert(newItem->path,newItem);
                nItems<<newItem;
                ++nCount;
                if(oFolder) q_ptr->endInsertRows();

            }
        }
        else
        {
            if (fileName == "." || fileName == "..")continue;
            if(!currentPaths.contains(filePath))
            {
                PlayListItem *folderCollection = new PlayListItem();
                folderCollection->title = fileName;
                folderCollection->folderPath = filePath;
                folderCollection->addTime = QDateTime::currentDateTime().toSecsSinceEpoch();
                int c = refreshFolder(folderCollection, nItems);
                if(c>0)
                {
                    if(oFolder) q_ptr->beginInsertRows(fIndex, folderItem->children->size(), folderItem->children->size());
                    folderCollection->moveTo(folderItem);
                    if(oFolder) q_ptr->endInsertRows();
                }
                else delete folderCollection;
                nCount += c;
            }
        }
    }
    return nCount;
}

QString PlayListPrivate::setCollectionTitle(QList<PlayListItem *> &list)
{
    int minTitleLen=INT_MAX;
    PlayListItem *suffixItem=nullptr;
    const int minLength = 5;
    const QString defaultTitle(QObject::tr("new collection"));
    for(PlayListItem *item:list)
    {
        if(!item->animeTitle.isEmpty())
            return item->animeTitle;
        if(item->title.length()<minTitleLen)
        {
            minTitleLen=item->title.length();
            suffixItem=item;
        }
    }
    if(list.count() < 2)return defaultTitle;
    if(minTitleLen < minLength)return defaultTitle;
    std::function<int (const QString &,const QString &,int *)> kmpMatchLen
            = [](const QString &suffix,const QString &str,int *next)
    {
        int maxMatchLen=0,i=0,j=0;
        for (; i < str.length(); ++i)
        {
            while (j > -1 && str[i] != suffix[j])
                j = next[j];
            if (j == suffix.length() - 1)
                return j;
            j = j + 1;
            if(j > maxMatchLen)maxMatchLen=j;
        }
        return maxMatchLen;
    };
    std::function<int * (const QString &)> getNext
            =[](const QString &suffix)
    {
        int i = 0, j = -1;
        int *next = new int[suffix.length()];
        for (; i < suffix.length(); ++i) {
            next[i] = j;
            while (j > -1 && suffix[i] != suffix[j])
                j = next[j];
            j = j + 1;
        }
        return next;
    };
    Q_ASSERT(suffixItem!=nullptr);
    QString &title=suffixItem->title;
    QString matchStr;
    for(int i=minTitleLen;i>=minLength;i--)
    {
        QString suffix(title.right(i));
        int *next=getNext(suffix);
        int tmpMaxMinMatch=INT_MAX;
        for(PlayListItem *item:list)
        {
            if(item!=suffixItem)
            {
                int len=kmpMatchLen(suffix,item->title,next);
                if(len<tmpMaxMinMatch) tmpMaxMinMatch = len;
            }
        }
        delete[] next;
        if(tmpMaxMinMatch>matchStr.length())
            matchStr=suffix.left(tmpMaxMinMatch);
    }
    return matchStr.length() < minLength? defaultTitle : matchStr;
}

void PlayListPrivate::dumpItem(QJsonArray &array, PlayListItem *item, QHash<QString, QString> &mediaHash)
{
    for(PlayListItem *child:*item->children)
    {
        QJsonObject itemObj;
        itemObj.insert("text",child->title);
        if(child->marker!=PlayListItem::M_NONE)
        {
            itemObj.insert("marker", (int)child->marker);
        }
        if(child->children)
        {
            QJsonArray childArray;
            dumpItem(childArray,child,mediaHash);
            itemObj.insert("nodes",childArray);
        }
        else
        {
            QString pathHash(QCryptographicHash::hash(child->path.toUtf8(),QCryptographicHash::Md5).toHex());
            itemObj.insert("mediaId",pathHash);
            itemObj.insert("danmuPool",child->poolID);
            itemObj.insert("playTime",child->playTime);
            itemObj.insert("playTimeState",child->playTimeState);
            itemObj.insert("animeName", child->animeTitle);
            static QString nodeColors[3]={"#333","#428bca","#a4a2a2"};
            itemObj.insert("color",nodeColors[child->playTimeState]);
            mediaHash.insert(pathHash,child->path);
        }
        array.append(itemObj);
    }
}
