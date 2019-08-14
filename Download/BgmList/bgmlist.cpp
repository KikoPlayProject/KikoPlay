#include "bgmlist.h"
#include "globalobjects.h"
#include "Common/network.h"
#include "Common/threadtask.h"
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

void BgmList::setSeason(const QString &id)
{
    if(!bgmSeasons.contains(id)) return;
    if(curSeason && curSeason->id==id) return;
    beginResetModel();
    if(needSave && curSeason)
    {
        saveLocal(*curSeason);
        needSave=false;
    }
    auto &season = bgmSeasons[id];
    if(season.bgmList.isEmpty())
    {
        loadLocal(season);
    }
    if(!season.latestVersion.isEmpty() && season.latestVersion!=season.version)
    {
        fetchBgmList(season);
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
        if(!curSeason->latestVersion.isEmpty() && curSeason->latestVersion!=curSeason->version)
        {
            beginResetModel();
            fetchBgmList(*curSeason);
            endResetModel();
        }
    }
}

QVariant BgmList::data(const QModelIndex &index, int role) const
{
    if(!curSeason) return QVariant();
    if (!index.isValid()) return QVariant();
    int col=index.column();
    const BgmItem &item=curSeason->bgmList.at(index.row());
    switch (role)
    {
    case Qt::DisplayRole:
    {
        if(col==0)
        {
            return item.title;
        }
        else if(col==1)
        {
            return QString("%1, %2").arg(item.showDate).arg(item.time);
        }
        else if(col==2)
        {
            return item.sitesName;
        }
        break;
    }
    case Qt::TextAlignmentRole:
    {
        return Qt::AlignCenter;
    }
    case Qt::ForegroundRole:
    {
        if(item.isNew) return QBrush(QColor(0,155,255));
        else return QBrush(QColor(100,100,100));
    }
    case Qt::CheckStateRole:
    {
        if(col==3)
            return item.focus?Qt::Checked:Qt::Unchecked;
        break;
    }
    case Qt::ToolTipRole:
    {
        if(col==0)
            return item.title;
        else if(col==2)
            return item.sitesName;
        break;
    }
    }
    return QVariant();
}

bool BgmList::setData(const QModelIndex &index, const QVariant &value, int )
{
    if(!curSeason) return false;
    if(index.column()==3)
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
    static QStringList headers({tr("Title"),tr("ShowDate/Time"),tr("AirSites"),tr("Focus")});
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
    ThreadTask task(GlobalObjects::workThread);
    emit bgmStatusUpdated(2,tr("Fetching Info..."));
    QString ret = task.Run([this](){
        try
        {
            QJsonObject archive(Network::toJson(Network::httpGet("https://bgmlist.com/tempapi/archive.json",QUrlQuery())).object());
            archive=archive.value("data").toObject();
            QStringList seasons;
            for(auto iter=archive.begin();iter!=archive.end();++iter)
            {
                QString year(iter.key());
                QJsonObject months(iter.value().toObject());
                for(auto mIter=months.begin(); mIter!=months.end(); ++mIter)
                {
                    QString month(mIter.key());
                    if(month.length()==1) month="0"+month;
                    QJsonObject bgmSObj(mIter.value().toObject());
                    QString id(year+'-'+month);
                    seasons<<id;
                    if(bgmSeasons.contains(id))
                    {
                        bgmSeasons[id].path=bgmSObj.value("path").toString();
                        bgmSeasons[id].latestVersion=bgmSObj.value("version").toString();
                    }
                    else
                    {
                        BgmSeason bs;
                        bs.id=id;
                        bs.path=bgmSObj.value("path").toString();
                        bs.latestVersion=bgmSObj.value("version").toString();
                        bgmSeasons.insert(id, bs);
                    }
                }
            }
            seasons.sort();
            this->seasons=seasons;
            return QString();

        } catch (Network::NetworkError &err) {
            return err.errorInfo;
        }
    }).toString();
    if(ret.isEmpty())
    {
        seasonsDownloaded = true;
        emit seasonsUpdated();
        return true;
    }
    emit bgmStatusUpdated(3,tr("Fetch Failed: %1").arg(ret));
    seasonsDownloaded = false;
    return false;
}

bool BgmList::fetchBgmList(BgmSeason &season)
{
    ThreadTask task(GlobalObjects::workThread);
    emit bgmStatusUpdated(2,tr("Fetching Info..."));
    QString ret = task.Run([&season,this](){
        try
        {
            QString content(Network::httpGet(season.path, QUrlQuery()));
            QJsonObject bgmListObj(Network::toJson(content).object());
            season.newBgmCount=0;
            int lastPos = seasons.indexOf(season.id)-1;
            if(lastPos>=0 && season.focusSet.isEmpty())
            {
                QString lastId(seasons.at(lastPos));
                if(bgmSeasons[lastId].bgmList.isEmpty()) loadLocal(bgmSeasons[lastId]);
                season.focusSet=bgmSeasons[lastId].focusSet;
            }
            QList<BgmItem> tmpBgms;
            QSet<QString> titleSet;
            for(auto iter=bgmListObj.constBegin();iter!=bgmListObj.constEnd();++iter)
            {
                QJsonObject bgmObj(iter->toObject());
                BgmItem item;
                item.isNew=bgmObj.value("newBgm").toBool();
                if(item.isNew) season.newBgmCount++;
                item.title=bgmObj.value("titleCN").toString();
                titleSet<<item.title;
                item.week=bgmObj.value("weekDayCN").toInt();
                item.bgmId=bgmObj.value("bgmId").toInt();
                item.showDate=bgmObj.value("showDate").toString();
                QString time(bgmObj.value("timeCN").toString().isEmpty()?bgmObj.value("timeJP").toString():bgmObj.value("timeCN").toString());
                item.time=time.left(2)+":"+time.right(2);
                item.focus=season.focusSet.contains(item.title);
                QJsonArray sites(bgmObj.value("onAirSite").toArray());
                QStringList sitesList;
                for (auto value:sites)
                {
                    QString url(value.toString());
                    item.onAirURL<<url;
                    QString name(getSiteName(url));
                    if(!name.isEmpty()) sitesList<<getSiteName(url);
                }
                item.sitesName=sitesList.join('|');
                tmpBgms<<item;
            }
            season.bgmList.swap(tmpBgms);
            season.focusSet.intersect(titleSet);
            season.version=season.latestVersion;
            saveLocal(season);
            return QString();
        } catch (Network::NetworkError &err)
        {
            return err.errorInfo;
        }
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
    QFile localFile(GlobalObjects::dataPath+"/bgmlist/"+season.id+".xml");
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
            else if(localStart && name=="version")
            {
                season.version=reader.readElementText();
            }
            else if(name=="list")
            {
                listStart=true;
            }
            else if(listStart && name=="item")
            {
                BgmItem item;
                item.title= reader.attributes().value("title").toString();
                item.time= reader.attributes().value("time").toString();
                item.week=reader.attributes().value("week").toInt();
                item.bgmId=reader.attributes().value("bgmId").toInt();
                item.showDate=reader.attributes().value("showDate").toString();
                item.isNew=reader.attributes().value("isNew").toInt();
                if(item.isNew) season.newBgmCount++;
                item.onAirURL=reader.attributes().value("onAirURL").toString().split(';');
                item.focus=season.focusSet.contains(item.title);
                QStringList onAirURL(reader.attributes().value("onAirURL").toString().split(';'));
                QStringList sites;
                for(const QString &url:onAirURL)
                {
                    QString name(getSiteName(url));
                    if(!name.isEmpty()) sites<<getSiteName(url);
                }
                item.sitesName=sites.join('|');
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
    QFile localFile(GlobalObjects::dataPath+"/bgmlist/"+season.id+".xml");
    bool ret=localFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&localFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("bgmInfo");
    writer.writeStartElement("local");
    writer.writeStartElement("version");
    writer.writeCharacters(season.version);
    writer.writeEndElement();
    writer.writeStartElement("focus");
    writer.writeCharacters(season.focusSet.toList().join(";;"));
    writer.writeEndElement();
    writer.writeEndElement();//local
    writer.writeStartElement("list");
    for(auto &item :season.bgmList)
    {
        writer.writeStartElement("item");
        writer.writeAttribute("title", item.title);
        writer.writeAttribute("time", item.time);
        writer.writeAttribute("week", QString::number(item.week));
        writer.writeAttribute("bgmId", QString::number(item.bgmId));
        writer.writeAttribute("showDate", item.showDate);
        writer.writeAttribute("isNew", QString::number(item.isNew?1:0));
        writer.writeAttribute("onAirURL", item.onAirURL.join(';'));
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
        QString id(fileName.left(7));
        seasons.append(id);
        BgmSeason bs;
        bs.id=id;
        bgmSeasons.insert(id, bs);
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
    if(week>6 || item.week==week)
    {
        return (onlyFocus?item.focus:true) && (onlyNew?item.isNew:true);
    }
    else
    {
        return false;
    }
}

