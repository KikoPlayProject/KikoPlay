#include "bgmlist.h"
#include "globalobjects.h"
#include "Common/network.h"
#include "Common/threadtask.h"
#include "Script/scriptmanager.h"
#include "Script/scriptlogger.h"
#include <QBrush>
BgmList::BgmList(QObject *parent) : QAbstractItemModel (parent),inited(false),needSave(false),
    seasonsDownloaded(false), curSeasonDownloaded(false),curSeason(nullptr)
{
    QFile sitesFile(":/res/onAirSites.json");
    sitesFile.open(QFile::ReadOnly);
    if(sitesFile.isOpen())
    {
        QJsonObject sObj(Network::toJson(sitesFile.readAll()).object());
        for(auto iter=sObj.constBegin();iter!=sObj.constEnd();++iter)
        {
            QJsonObject site(iter.value().toObject());
            sitesName.insert(site.value("urlTemplate").toString(),site.value("title").toString());
        }
    }
    getLocalSeasons();
}

BgmList::~BgmList()
{
    if(needSave && curSeason) saveLocal(*curSeason);
}

void BgmList::init()
{
    if(inited) return;
    fetchMetaInfo();
    inited=true;
}

void BgmList::setSeason(const QString &title)
{
    if(!bgmSeasons.contains(title)) return;
    if(curSeason && curSeason->title==title) return;
    beginResetModel();
    if(needSave && curSeason)
    {
        saveLocal(*curSeason);
        needSave=false;
    }
    auto &season = bgmSeasons[title];
    if(season.bgmList.isEmpty())
    {
        if(!loadLocal(season))
        {
            fetchBgmList(season);
        }
    }
    curSeason=&season;
    emit bgmStatusUpdated(1,QString("%1/%2 %3").arg(season.newBgmCount).arg(season.bgmList.count()).
                          arg(season.focusSet.isEmpty()?"":tr(" Focus: %1").arg(season.focusSet.count())));
    endResetModel();
}

void BgmList::refresh()
{
    if(!seasonsDownloaded)
    {
        if(!fetchMetaInfo()) return;
    }
    if(!curSeasonDownloaded && curSeason)
    {
        beginResetModel();
        fetchBgmList(*curSeason);
        endResetModel();
    }
}

void BgmList::setHoverColor(const QColor &color)
{
    beginResetModel();
    hoverColor = color;
    endResetModel();
}

void BgmList::setNormColor(const QColor &color)
{
    beginResetModel();
    normColor = color;
    endResetModel();
}

QVariant BgmList::data(const QModelIndex &index, int role) const
{
    if(!curSeason) return QVariant();
    if (!index.isValid()) return QVariant();
    const BgmItem &item=curSeason->bgmList.at(index.row());
    Columns col=static_cast<Columns>(index.column());
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col==Columns::TITLE)
        {
            return item.title;
        }
        else if(col==Columns::DATETIME)
        {
            if(!item.airTime.isEmpty())
                return QString("%1, %2").arg(item.airDate).arg(item.airTime);
            return item.airDate;
        }
        else if(col==Columns::AIRSITES)
        {
            return item.onAirSites.join('|');
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        return Qt::AlignCenter;
    }
    case Qt::ForegroundRole:
    {
        if(item.isNew) return QBrush(hoverColor);
        else return QBrush(normColor);
    }
    case Qt::CheckStateRole:
    {
        if(col==Columns::FOCUS)
            return item.focus?Qt::Checked:Qt::Unchecked;
        break;
    }
    case Qt::ToolTipRole:
    {
        if(col==Columns::TITLE)
            return item.title;
        else if(col==Columns::AIRSITES)
            return item.onAirSites.join('|');
        break;
    }
    }
    return QVariant();
}

bool BgmList::setData(const QModelIndex &index, const QVariant &value, int )
{
    if(!curSeason) return false;
    Columns col=static_cast<Columns>(index.column());
    if(col==Columns::FOCUS)
    {
        BgmItem &item=curSeason->bgmList[index.row()];
        if(value==Qt::Checked)
        {
            curSeason->focusSet<<item.title;
            item.focus=true;
        }
        else
        {
            curSeason->focusSet.remove(item.title);
            item.focus=false;
        }
        needSave=true;
        emit bgmStatusUpdated(1,QString("%1/%2 %3").arg(curSeason->newBgmCount).arg(curSeason->bgmList.count()).
                              arg(curSeason->focusSet.isEmpty()?"":tr(" Focus: %1").arg(curSeason->focusSet.count())));
        return true;
    }
    return false;
}

QVariant BgmList::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role == Qt::DisplayRole&&orientation == Qt::Horizontal)
    {
        if(section<4)return headers.at(section);
    }
    else if(role==Qt::TextAlignmentRole)
    {
        if(section==3) return static_cast<int>(Qt::AlignLeft|Qt::AlignVCenter);
        return Qt::AlignCenter;
    }
    return QVariant();
}

bool BgmList::fetchMetaInfo()
{
    const auto &calendarScripts = GlobalObjects::scriptManager->scripts(ScriptType::BGM_CALENDAR);
    if(calendarScripts.empty())
    {
        emit bgmStatusUpdated(3,tr("Bangumi Calendar script not exist"));
        seasonsDownloaded = false;
        return false;
    }
    auto bgmCalendar = calendarScripts.first().staticCast<BgmCalendarScript>();
    ScriptLogger::instance()->appendInfo(tr("Select Bangumi Calendar: %1").arg(bgmCalendar->name()), bgmCalendar->id());
    ThreadTask task(GlobalObjects::workThread);
    emit bgmStatusUpdated(2, tr("Fetching Info..."));
    QList<BgmSeason> bgmSeasonList;
    ScriptState state = task.Run([&bgmCalendar, &bgmSeasonList](){
        return QVariant::fromValue(bgmCalendar->getSeason(bgmSeasonList));
    }).value<ScriptState>();
    if(!state)
    {
        emit bgmStatusUpdated(3,tr("Fetch Failed: %1").arg(state.info));
        seasonsDownloaded = false;
        return false;
    }
    seasons.clear();
    bgmSeasons.clear();
    for(auto &s : bgmSeasonList)
    {
        seasons.append(s.title);
        bgmSeasons.insert(s.title, s);
    }
    seasonsDownloaded = true;
    emit seasonsUpdated();
    return true;
}

bool BgmList::fetchBgmList(BgmSeason &season)
{
    const auto &calendarScripts = GlobalObjects::scriptManager->scripts(ScriptType::BGM_CALENDAR);
    if(calendarScripts.empty())
    {
        emit bgmStatusUpdated(3,tr("Bangumi Calendar script not exist"));
        curSeasonDownloaded = false;
        return false;
    }
    auto bgmCalendar = calendarScripts.first().staticCast<BgmCalendarScript>();
    ScriptLogger::instance()->appendInfo(tr("Select Bangumi Calendar: %1").arg(bgmCalendar->name()), bgmCalendar->id());
    ThreadTask task(GlobalObjects::workThread);
    emit bgmStatusUpdated(2,tr("Fetching Info..."));
    QString ret = task.Run([&season, this, &bgmCalendar](){
        ScriptState state = bgmCalendar->getBgmList(season);
        if(!state) return state.info;
        int lastPos = seasons.indexOf(season.title)-1;
        if(lastPos>=0 && season.focusSet.isEmpty())
        {
            QString lastTitle(seasons.at(lastPos));
            if(bgmSeasons[lastTitle].bgmList.isEmpty()) loadLocal(bgmSeasons[lastTitle]);
            season.focusSet.unite(bgmSeasons[lastTitle].focusSet);
        }
        QSet<QString> titleSet;
        for(const BgmItem &bgm : season.bgmList)
        {
            titleSet.insert(bgm.title);
        }
        season.focusSet.intersect(titleSet);
        saveLocal(season);
        return QString();
    }).toString();
    if(ret.isEmpty())
    {
        curSeasonDownloaded=true;
        emit bgmStatusUpdated(1,QString("%1/%2 %3").arg(season.newBgmCount).arg(season.bgmList.count()).
                              arg(season.focusSet.isEmpty()?"":tr(" Focus: %1").arg(season.focusSet.count())));

    }
    else
    {
        curSeasonDownloaded=false;
        emit bgmStatusUpdated(3,tr("Fetch Failed: %1").arg(ret));
    }
    return curSeasonDownloaded;
}

bool BgmList::loadLocal(BgmSeason &season)
{
    QFile localFile(GlobalObjects::dataPath+"/bgmlist/"+season.title+".xml");
    localFile.open(QFile::ReadOnly);
    if(!localFile.isOpen()) return false;
    season.newBgmCount=0;
    QXmlStreamReader reader(&localFile);
    bool localStart=false,listStart=false;
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name=="local")
            {
                localStart=true;
            }
            else if(localStart && name=="focus")
            {
                QStringList focusList(reader.readElementText().split(";;",QString::SkipEmptyParts));
                for(const QString &title:focusList)
                {
                    season.focusSet<<title;
                }
            }
            else if(name=="list")
            {
                listStart=true;
            }
            else if(listStart && name=="item")
            {
                BgmItem item;
                item.title= reader.attributes().value("title").toString();
                item.airTime= reader.attributes().value("time").toString();
                item.weekDay=reader.attributes().value("week").toInt();
                item.bgmId=reader.attributes().value("bgmId").toString();
                item.airDate=reader.attributes().value("showDate").toString();
                item.isNew=reader.attributes().value("isNew").toInt();
                if(item.isNew) season.newBgmCount++;
                QString airUrls = reader.attributes().value("onAirURL").toString();
                if(!airUrls.isEmpty())
                {
                    item.onAirURLs=airUrls.split(';');
                }
                item.focus=season.focusSet.contains(item.title);
				QString airSites = reader.attributes().value("onAirSite").toString();
				if (!airSites.isEmpty())
				{
					item.onAirSites = airSites.split(';', Qt::SkipEmptyParts);
				}
                if(!item.onAirURLs.empty() && item.onAirSites.isEmpty())
                {
                    std::transform(item.onAirURLs.begin(), item.onAirURLs.end(),
                                   std::back_inserter(item.onAirSites), [this](const QString &url){
                        return getSiteName(url);
                    });
                }
                season.bgmList<<item;
            }
        }
        else if(reader.isEndElement())
        {
            QStringRef name=reader.name();
            if(name=="local") localStart=false;
            else if(name=="list") listStart=false;
        }
        reader.readNext();
    }
    return true;
}

void BgmList::saveLocal(const BgmSeason &season)
{
    QDir dir;
    if(!dir.exists(GlobalObjects::dataPath+"/bgmlist/"))
    {
        dir.mkpath(GlobalObjects::dataPath+"/bgmlist/");
    }
    QFile localFile(GlobalObjects::dataPath+"/bgmlist/"+season.title+".xml");
    bool ret=localFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&localFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("bgmInfo");
    writer.writeStartElement("local");
    writer.writeStartElement("focus");
    writer.writeCharacters(season.focusSet.toList().join(";;"));
    writer.writeEndElement();
    writer.writeEndElement();//local
    writer.writeStartElement("list");
    for(auto &item :season.bgmList)
    {
        writer.writeStartElement("item");
        writer.writeAttribute("title", item.title);
        writer.writeAttribute("time", item.airTime);
        writer.writeAttribute("week", QString::number(item.weekDay));
        writer.writeAttribute("bgmId", item.bgmId);
        writer.writeAttribute("showDate", item.airDate);
        writer.writeAttribute("isNew", QString::number(item.isNew?1:0));
        writer.writeAttribute("onAirURL", item.onAirURLs.join(';'));
        writer.writeAttribute("onAirSite", item.onAirSites.join(';'));
        writer.writeEndElement();
    }
    writer.writeEndElement();//list
    writer.writeEndElement();//bgmInfo
    writer.writeEndDocument();
}

bool BgmList::getLocalSeasons()
{
    QDir bgmlistFolder(GlobalObjects::dataPath+"/bgmlist");
    bgmlistFolder.setSorting(QDir::Name);
    QRegExp re("\\d{4}-\\d{2}\\.xml");
    for (QFileInfo fileInfo : bgmlistFolder.entryInfoList())
    {
        if(!fileInfo.isFile()) continue;
        QString fileName = fileInfo.fileName();
        if(re.indexIn(fileName)==-1) continue;
        if(re.matchedLength()!=fileName.length()) continue;
        QString title(fileName.left(7));
        seasons.append(title);
        BgmSeason bs;
        bs.title=title;
        bgmSeasons.insert(title, bs);
    }
    return true;
}

QString BgmList::getSiteName(const QString &url)
{
    auto iter=sitesName.lowerBound(url);
    if(iter!=sitesName.begin())
    {
        --iter;
        if(url.startsWith(iter.key()))
        {
           return iter.value();
        }
        else
        {
            return "";
        }
    }
    else
    {
        return "";
    }
}

void BgmListFilterProxyModel::setWeekFilter(int week)
{
    this->week=week;
    invalidateFilter();
}

void BgmListFilterProxyModel::setFocusFilter(bool onlyFocus)
{
    this->onlyFocus=onlyFocus;
    invalidateFilter();
}

void BgmListFilterProxyModel::setNewBgmFilter(bool onlyNew)
{
    this->onlyNew=onlyNew;
    invalidateFilter();
}

bool BgmListFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &) const
{
    const BgmItem &item=static_cast<BgmList *>(sourceModel())->bgmList().at(source_row);
    if(week>6 || item.weekDay==week)
    {
        return (onlyFocus?item.focus:true) && (onlyNew?item.isNew:true);
    }
    else
    {
        return false;
    }
}

