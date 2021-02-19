#include "animefilterproxymodel.h"
#include "animemodel.h"
#include "labelmodel.h"
#include "Common/notifier.h"
#include "globalobjects.h"

AnimeFilterProxyModel::AnimeFilterProxyModel(AnimeModel *srcModel, QObject *parent):QSortFilterProxyModel(parent),filterType(0)
{
    QObject::connect(srcModel, &AnimeModel::animeCountInfo,this, &AnimeFilterProxyModel::refreshAnimeCount);
}

void AnimeFilterProxyModel::setFilter(int type, const QString &str)
{
    filterType=type;
    setFilterRegExp(str);
    static_cast<AnimeModel *>(sourceModel())->showStatisMessage();
}

void AnimeFilterProxyModel::setTags(SelectedLabelInfo &&selectedLabels)
{
    filterLabels = selectedLabels;
    invalidateFilter();
    static_cast<AnimeModel *>(sourceModel())->showStatisMessage();
}

void AnimeFilterProxyModel::refreshAnimeCount(int cur, int total)
{
    if(cur<total)
    {
        emit animeMessage(tr("Current: %1/%2 Loaded: %2/%3").arg(rowCount()).arg(cur).arg(total), true);
    }
    else
    {
        emit animeMessage(tr("Current: %1/%2").arg(rowCount()).arg(cur), false);
    }
}

bool AnimeFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
     QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
     AnimeModel *model = static_cast<AnimeModel *>(sourceModel());
     Anime *anime = model->getAnime(index);

     if(!filterLabels.scriptTags.isEmpty() && !filterLabels.scriptTags.contains(anime->scriptId())) return false;

     QString animeTime(anime->airDate().left(7));
     if(!filterLabels.timeTags.isEmpty() && !filterLabels.timeTags.contains(animeTime))return false;

     if(!filterLabels.epPathTags.isEmpty())
     {
         bool contains = false;
         for(const QString &tag : filterLabels.epPathTags)
         {
             auto iter = GlobalObjects::animeLabelModel->epTags().lowerBound(tag);
             while(iter != GlobalObjects::animeLabelModel->epTags().end() && iter.key().startsWith(tag))
             {
                 if(iter.key()==tag || iter.key().startsWith(tag+'/'))
                 {
                     if(iter.value().contains(anime->name()))
                     {
                         contains = true;
                         break;
                     }
                 }
                 ++iter;
             }
             if(contains) break;
         }
         if(!contains) return false;
     }

     if(!filterLabels.customPrefixTags.isEmpty())
     {
         bool contains = false;
         for(const QString &tag : filterLabels.customPrefixTags)
         {
             auto iter = GlobalObjects::animeLabelModel->customTags().lowerBound(tag);
             while(iter != GlobalObjects::animeLabelModel->customTags().end() && iter.key().startsWith(tag))
             {
                 if(iter.key()==tag || iter.key().startsWith(tag+'/'))
                 {
                     if(iter.value().contains(anime->name()))
                     {
                         contains = true;
                         break;
                     }
                 }
				 ++iter;
             }
             if(contains) break;
         }
         if(!contains) return false;
     }

     for(const QString &tag : filterLabels.customTags)
     {
         if(!GlobalObjects::animeLabelModel->customTags()[tag].contains(anime->name()))
             return false;
     }

     switch (filterType)
     {
     case 0://title
         return anime->name().contains(filterRegExp());
     case 1://summary
         return anime->description().contains(filterRegExp());
     case 2://staff
     {
         for(auto iter=anime->staffList().cbegin();iter!=anime->staffList().cend();++iter)
         {
             if((*iter).second.contains(filterRegExp()))
                 return true;
         }
         return false;
     }
     case 3://crt
     {
         for(auto iter=anime->crList().cbegin();iter!=anime->crList().cend();++iter)
         {
             if((*iter).name.contains(filterRegExp()) || (*iter).actor.contains(filterRegExp()))
                 return true;
         }
         return false;
     }
     }
     return true;
}
