#include "animefilterproxymodel.h"
#include "animelibrary.h"
#include "animemodel.h"
#include "labelmodel.h"
#include "globalobjects.h"

AnimeFilterProxyModel::AnimeFilterProxyModel(QObject *parent):QSortFilterProxyModel(parent),filterType(0)
{
    QObject::connect(GlobalObjects::library->animeModel, &AnimeModel::animeCountInfo,
                     this, &AnimeFilterProxyModel::refreshAnimeCount);
}

void AnimeFilterProxyModel::setFilterType(int type)
{
    filterType=type;
    invalidateFilter();
    GlobalObjects::library->animeModel->showStatisMessage();
}
void AnimeFilterProxyModel::setTags(const QStringList &tagList)
{
    tagFilterList=tagList;
    invalidateFilter();
    GlobalObjects::library->animeModel->showStatisMessage();
}

void AnimeFilterProxyModel::setTime(const QSet<QString> &timeSet)
{
    timeFilterSet=timeSet;
    invalidateFilter();
    GlobalObjects::library->animeModel->showStatisMessage();
}

void AnimeFilterProxyModel::refreshAnimeCount(int cur, int total)
{
    emit animeMessage(tr("Current: %1/%2 Loaded: %2/%3").arg(rowCount()).arg(cur).arg(total),
                      total==cur?PopMessageFlag::PM_OK:PopMessageFlag::PM_INFO,total!=cur);
}

bool AnimeFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
     QModelIndex index = sourceModel()->index(source_row, 0, source_parent);
     Anime *anime=GlobalObjects::library->animeModel->getAnime(index,false);
     QString animeTime(anime->date.left(7));
     if(!timeFilterSet.isEmpty() && !timeFilterSet.contains(animeTime.isEmpty()?tr("Unknown"):animeTime))return false;
     for(const QString &tag:tagFilterList)
     {
         if(!GlobalObjects::library->labelModel->getTags()[tag].contains(anime->title))
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
