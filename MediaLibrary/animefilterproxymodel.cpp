#include "animefilterproxymodel.h"
#include "animemodel.h"
#include "labelmodel.h"

AnimeFilterProxyModel::AnimeFilterProxyModel(AnimeModel *srcModel, QObject *parent):QSortFilterProxyModel(parent),filterType(0), orderType(O_AddTime), ascending(false)
{
    QObject::connect(srcModel, &AnimeModel::animeCountInfo,this, &AnimeFilterProxyModel::refreshAnimeCount);
}

void AnimeFilterProxyModel::setFilter(int type, const QString &str)
{
    filterType=type;
    setFilterRegExp(str);
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    static_cast<AnimeModel *>(sourceModel())->showStatisMessage();
}

void AnimeFilterProxyModel::setTags(SelectedLabelInfo &&selectedLabels)
{
    filterLabels = selectedLabels;
    invalidateFilter();
    static_cast<AnimeModel *>(sourceModel())->showStatisMessage();
}

void AnimeFilterProxyModel::setOrder(AnimeFilterProxyModel::OrderType oType)
{
    if(orderType==oType) return;
    orderType = oType;
    invalidate();
    sort(0);
}

void AnimeFilterProxyModel::setAscending(bool on)
{
    if(ascending==on) return;
    ascending = on;
    invalidate();
    sort(0);
}

void AnimeFilterProxyModel::refreshAnimeCount(int cur, int total)
{
    int filterCount = rowCount();
    if (filterCount < cur)
    {
        emit animeMessage(tr("%1/%2 items").arg(rowCount()).arg(total), cur < total);
    }
    else
    {
        emit animeMessage(tr("%1 items").arg(total), false);
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
             auto iter = LabelModel::instance()->epTags().lowerBound(tag);
             while(iter != LabelModel::instance()->epTags().end() && iter.key().startsWith(tag))
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
             auto iter = LabelModel::instance()->customTags().lowerBound(tag);
             while(iter != LabelModel::instance()->customTags().end() && iter.key().startsWith(tag))
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
         if(!LabelModel::instance()->customTags()[tag].contains(anime->name()))
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

bool AnimeFilterProxyModel::lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const
{
    AnimeModel *model = static_cast<AnimeModel *>(sourceModel());
    Anime *animeL = model->getAnime(source_left), *animeR = model->getAnime(source_right);
    static QCollator comparer;
    switch (orderType)
    {
    case O_AddTime:
        return ascending?animeL->addTime()<animeR->addTime():
                         animeL->addTime()>animeR->addTime();
    case O_Title:
        return ascending?comparer.compare(animeL->name(), animeR->name())<0:
                         comparer.compare(animeL->name(), animeR->name())>0;
    case O_AirDate:
        return ascending?animeL->airDate()<animeR->airDate():
                         animeL->airDate()>animeR->airDate();
    }
    return QSortFilterProxyModel::lessThan(source_left, source_right);
}
