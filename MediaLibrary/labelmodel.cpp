#include "labelmodel.h"
#include "animelibrary.h"
#include "globalobjects.h"
#include <QBrush>
#define TYPELEVEL 1
#define LABEL_TIME 2
#define LABEL_TAG 3
#define CountRole Qt::UserRole+1

LabelModel::LabelModel(AnimeLibrary *library) : QAbstractItemModel(library)
{
    QObject::connect(library,&AnimeLibrary::addTagsTo,this,[this](const QString &title, const QStringList &tags){
        for(const QString &tag:tags)
        {
            if(!tagMap.contains(tag))
            {
                tagMap.insert(tag,QSet<QString>({title}));
                int insPos = std::lower_bound(tagList.begin(),tagList.end(),1,[](const QPair<QString,int> &pair, int animeCount){
                    return pair.second>=animeCount;
                })-tagList.begin();
                beginInsertRows(createIndex(1,0,TYPELEVEL),insPos,insPos);
                tagList.insert(insPos,QPair<QString,int>(tag,1));
                endInsertRows();
            }
            else
            {
                if(tagMap[tag].contains(title)) continue;
                tagMap[tag].insert(title);
                for(int i=0;i<tagList.size();++i)
                {
                    if(tagList[i].first==tag)
                    {
                        ++tagList[i].second;
                        QModelIndex index(createIndex(i,0,LABEL_TAG));
                        emit dataChanged(index,index);
                        if(i>0)
                        {
                            int nPos=i-1;
                            while(tagList.at(nPos).second<tagList[i].second)
                            {
                                --nPos;
                                if(nPos<0)
                                {
                                    break;
                                }
                            }
                            ++nPos;
                            if(nPos!=i)
                            {
                                QModelIndex lpIndex(createIndex(1,0,TYPELEVEL));
                                beginMoveRows(lpIndex,i,i,lpIndex,nPos);
                                tagList.move(i,nPos);
                                endMoveRows();
                            }
                        }
                        break;
                    }
                }
            }
        }
        GlobalObjects::library->saveTags(title,tags);
    });
    QObject::connect(library,&AnimeLibrary::addTimeLabel,this,[this](const QString &time, const QString &oldTime){
       int insPos = std::lower_bound(timeList.begin(),timeList.end(),time,[](const QPair<QString,int> &pair, const QString &insTime){
           return pair.first>insTime;
       })-timeList.begin();
       if(timeList.value(insPos).first==time)
       {
           ++timeList[insPos].second;
           QModelIndex index(createIndex(insPos,0,LABEL_TIME));
           emit dataChanged(index,index);
       }
       else
       {
           beginInsertRows(createIndex(0,0,TYPELEVEL),insPos,insPos);
           timeList.insert(insPos,QPair<QString,int>(time,1));
           endInsertRows();
       }
       if(!oldTime.isEmpty())
       {
           int oldPos = std::lower_bound(timeList.begin(),timeList.end(),oldTime,[](const QPair<QString,int> &pair, const QString &oldTime){
               return pair.first>oldTime;
           })-timeList.begin();
           if(timeList.value(oldPos).first==oldTime)
           {
               --timeList[oldPos].second;
               QModelIndex index(createIndex(oldPos,0,LABEL_TIME));
               if(timeList[oldPos].second==0)
               {
                   beginRemoveRows(index.parent(),index.row(),index.row());
                   timeList.removeAt(index.row());
                   endRemoveRows();
               }
               else
               {
                   emit dataChanged(index,index);
               }
           }
       }
    });
    QObject::connect(library,&AnimeLibrary::removeTagFrom,this,[this](const QString &title, const QString &tag){
        if(!tagMap.contains(tag)) return;
        if(!tagMap[tag].contains(title)) return;
        tagMap[tag].remove(title);
        if(tagMap[tag].size()==0)tagMap.remove(tag);
        int i=0;
        for(auto &pair:tagList)
        {
            if(pair.first==tag)
            {
                pair.second--;
                QModelIndex index(createIndex(i,0,LABEL_TAG));
                if(pair.second==0)
                {
                    beginRemoveRows(index.parent(),index.row(),index.row());
                    tagList.removeAt(index.row());
                    endRemoveRows();
                }
                else
                {
                    emit dataChanged(index,index);
                    if(i<tagList.size()-1)
                    {
                        int nPos=i+1;
                        while(tagList.at(nPos).second>tagList[i].second)
                        {
                            ++nPos;
                            if(nPos>=tagList.size())
                            {
                                break;
                            }
                        }
                        --nPos;
                        if(nPos!=i)
                        {
                            QModelIndex lpIndex(createIndex(1,0,TYPELEVEL));
                            beginMoveRows(lpIndex,i,i,lpIndex,nPos+1);
                            tagList.move(i,nPos);
                            endMoveRows();
                        }
                    }
                }
                break;
            }
            ++i;
        }
    });
    QObject::connect(library,&AnimeLibrary::removeTags,this,[this](const QString &title, const QString &time){
        QSet<QString> tagSet;
        for(auto iter=tagMap.begin();iter!=tagMap.end();)
        {
            if(iter.value().contains(title))
            {
                tagSet.insert(iter.key());
                iter.value().remove(title);
                if(iter.value().count()==0)
                    iter=tagMap.erase(iter);
                else
                    ++iter;
            }
            else
            {
                ++iter;
            }
        }
        int i=0;
        for(auto iter=tagList.begin();iter!=tagList.end();)
        {
            if(tagSet.contains((*iter).first))
            {
                --(*iter).second;
                QModelIndex index(createIndex(i,0,LABEL_TAG));
                if((*iter).second==0)
                {
                    beginRemoveRows(index.parent(),index.row(),index.row());
                    iter=tagList.erase(iter);
                    endRemoveRows();
                }
                else
                {
                    emit dataChanged(index,index);
                    ++i;
                    ++iter;          
                }
            }
            else
            {
                ++i;
                ++iter;
            }

        }
        QList<QPersistentModelIndex> persistentIndexList;
        persistentIndexList.append(QPersistentModelIndex(createIndex(1,0,TYPELEVEL)));
        emit layoutAboutToBeChanged(persistentIndexList);
        std::sort(tagList.begin(),tagList.end(),[](auto &tagPair1,auto &tagPair2){return tagPair1.second>tagPair2.second;});
        emit layoutChanged(persistentIndexList);

		if (time.isEmpty()) return;
        int insPos = std::lower_bound(timeList.begin(),timeList.end(),time,[](const QPair<QString,int> &pair, const QString &insTime){
            return pair.first>insTime;
        })-timeList.begin();
        if(timeList.value(insPos).first==time)
        {
            --timeList[insPos].second;
            QModelIndex index(createIndex(insPos,0,LABEL_TIME));
            if(timeList[insPos].second==0)
            {
                beginRemoveRows(index.parent(),index.row(),index.row());
                timeList.removeAt(index.row());
                endRemoveRows();
            }
            else
            {
                emit dataChanged(index,index);
            }
        }
    });
}

void LabelModel::refreshLabel()
{
    QMap<QString,int> timeMap;
    beginResetModel();
    timeList.clear();
    tagMap.clear();
    tagList.clear();
    GlobalObjects::library->loadTags(tagMap,timeMap);
    for(auto iter=tagMap.begin();iter!=tagMap.end();++iter)
    {
        tagList.append(QPair<QString,int>(iter.key(),iter.value().count()));
    }
    for(auto iter=timeMap.begin();iter!=timeMap.end();++iter)
    {
        timeList.append(QPair<QString,int>(iter.key(),iter.value()));
    }
    std::sort(tagList.begin(),tagList.end(),[](auto &tagPair1,auto &tagPair2){return tagPair1.second>tagPair2.second;});
    std::sort(timeList.begin(),timeList.end(),[](auto &time1,auto &time2){return time1.first>time2.first;});
    endResetModel();
}

void LabelModel::removeTag(const QModelIndex &index)
{
    if(!index.isValid()) return;
    if(index.internalId()!=LABEL_TAG) return;
    GlobalObjects::library->deleteTag(tagList.at(index.row()).first);
    tagMap.remove(tagList.at(index.row()).first);
    beginRemoveRows(index.parent(),index.row(),index.row());
    tagList.removeAt(index.row());
    endRemoveRows();
}

void LabelModel::selLabelList(const QModelIndexList &indexes, QStringList &tags, QSet<QString> &times)
{
    for(const QModelIndex &index:indexes)
    {
        if(!index.isValid()) continue;
        if(index.internalId()==LABEL_TIME)
        {
            times<<timeList.at(index.row()).first;
        }
        else if(index.internalId()==LABEL_TAG)
        {
            tags<<tagList.at(index.row()).first;
        }
    }
}

QModelIndex LabelModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) return QModelIndex();
    if(parent.isValid())
    {
        if(parent.row()==0)
        {
            return row<timeList.count()?createIndex(row,column,LABEL_TIME):QModelIndex();
        }
        else
        {
            return row<tagList.count()?createIndex(row,column,LABEL_TAG):QModelIndex();
        }
    }
    else
    {
        return row<2?createIndex(row,column,TYPELEVEL):QModelIndex();
    }
}

QModelIndex LabelModel::parent(const QModelIndex &child) const
{
    if (!child.isValid()) return QModelIndex();
    switch(child.internalId())
    {
    case LABEL_TIME:
        return createIndex(0, 0, TYPELEVEL);
    case LABEL_TAG:
        return createIndex(1, 0, TYPELEVEL);
    default:
        return QModelIndex();
    }
}

int LabelModel::rowCount(const QModelIndex &parent) const
{
    if (parent.column() > 0) return 0;
    if (parent.isValid())
    {
        if(parent.internalId()==TYPELEVEL)
            return parent.row()==0?timeList.count():tagList.count();
        return 0;
    }
    else
        return 2;
}

QVariant LabelModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(index.internalId()==TYPELEVEL)
        {
            if(index.row()==0)
                return tr("Air Date");
            else if(index.row()==1)
                return tr("Tag");
        }
        else if(index.internalId()==LABEL_TIME)
        {
            return timeList.at(index.row()).first;
        }
        else if(index.internalId()==LABEL_TAG)
        {
            return tagList.at(index.row()).first;
        }
        break;
    }
    case CountRole:
    {
        if(index.internalId()==LABEL_TIME)
        {
            return timeList.at(index.row()).second;
        }
        else if(index.internalId()==LABEL_TAG)
        {
            return tagList.at(index.row()).second;
        }
        break;
    }
    case Qt::ForegroundRole:
    {
        static QBrush labelBrs[]={QBrush(QColor(29,131,247)),QBrush(QColor(200,200,200))};
        return index.internalId()==TYPELEVEL?labelBrs[0]:labelBrs[1];
    }
    }
    return QVariant();
}
