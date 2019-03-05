#include "bgmlist.h"
#include "globalobjects.h"
#include "Common/network.h"
#include <QBrush>
BgmList::BgmList(QObject *parent) : QAbstractItemModel (parent),inited(false),needSave(false),showOnlyFocus(false),
    curWeek(-1),localYear(0),localMonth(0),newBgmCount(0)
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
    bgmWorker=new BgmWorker;
    bgmWorker->moveToThread(GlobalObjects::workThread);
    loadLocal();
}

BgmList::~BgmList()
{
    if(needSave) saveLocal();
}

void BgmList::init()
{
    if(inited) return;
    refreshData();
    inited=true;
}

void BgmList::refreshData(bool forceRefresh)
{
    emit bgmStatusUpdated(2,tr("Fetching Info..."));
    QEventLoop eventLoop;
    QObject::connect(bgmWorker,&BgmWorker::fetchDone, &eventLoop,[this,&eventLoop](const QString &errInfo, const QString &content, int nYear,int nMonth,const QString &nDataVersion){
        if(!errInfo.isEmpty())
        {
            emit bgmStatusUpdated(3,tr("Fetch Failed: %1").arg(errInfo));
        }
        else
        {
            if(!content.isEmpty())
            {
                newBgmCount=0;
                QJsonObject bgmListObj(Network::toJson(content).object());
                QList<BgmItem> tmpBgms;
                QSet<QString> titleSet;
                for(auto iter=bgmListObj.constBegin();iter!=bgmListObj.constEnd();++iter)
                {
                    QJsonObject bgmObj(iter->toObject());
                    BgmItem item;
                    item.isNew=bgmObj.value("newBgm").toBool();
                    if(item.isNew) newBgmCount++;
                    item.title=bgmObj.value("titleCN").toString();
                    titleSet<<item.title;
                    item.week=bgmObj.value("weekDayCN").toInt();
                    item.bgmId=bgmObj.value("bgmId").toInt();
                    item.showDate=bgmObj.value("showDate").toString();
                    QString time(bgmObj.value("timeCN").toString().isEmpty()?bgmObj.value("timeJP").toString():bgmObj.value("timeCN").toString());
                    item.time=time.left(2)+":"+time.right(2);
                    item.focus=focusSet.contains(item.title);
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
                beginResetModel();
                bgms.swap(tmpBgms);
                endResetModel();
                focusSet.intersect(titleSet);
                localYear=nYear;
                localMonth=nMonth;
                dataVersion=nDataVersion;
                saveLocal();
            }
            emit bgmStatusUpdated(1,tr("%1-%2 %3/%4 %5").arg(localYear).arg(localMonth).arg(newBgmCount).arg(bgms.count()).arg(focusSet.isEmpty()?"":tr(" Focus: %1").arg(focusSet.count())));
        }
        eventLoop.quit();
    });
    QMetaObject::invokeMethod(bgmWorker,[this,forceRefresh](){
        bgmWorker->fetchListInfo(localYear,localMonth,dataVersion,forceRefresh);
    },Qt::QueuedConnection);
    eventLoop.exec();
}

QVariant BgmList::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) return QVariant();
    int col=index.column();
    const BgmItem &item=bgms.at(index.row());
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
    if(index.column()==3)
    {
        BgmItem &item=bgms[index.row()];
        if(value==Qt::Checked)
        {
            focusSet<<item.title;
            item.focus=true;
        }
        else
        {
            focusSet.remove(item.title);
            item.focus=false;
        }
        needSave=true;
        emit bgmStatusUpdated(1,tr("%1-%2 %3/%4 %5").arg(localYear).arg(localMonth).arg(newBgmCount).arg(bgms.count()).arg(focusSet.isEmpty()?"":tr(" Focus: %1").arg(focusSet.count())));
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

bool BgmList::loadLocal()
{
    QFile localFile(GlobalObjects::dataPath+"bgmLocal.xml");
    localFile.open(QFile::ReadOnly);
    if(!localFile.isOpen()) return false;
    QXmlStreamReader reader(&localFile);
    bool localStart=false,listStart=false;
    beginResetModel();
    while(!reader.atEnd())
    {
        if(reader.isStartElement())
        {
            QStringRef name=reader.name();
            if(name=="local")
            {
                localStart=true;
            }
            else if(localStart && name=="year")
            {
                localYear=reader.readElementText().toInt();
            }
            else if(localStart && name=="month")
            {
                localMonth=reader.readElementText().toInt();
            }
            else if(localStart && name=="focus")
            {
                QStringList focusList(reader.readElementText().split(';',QString::SkipEmptyParts));
                for(const QString &title:focusList)
                {
                    focusSet<<title;
                }
            }
            else if(localStart && name=="version")
            {
                dataVersion=reader.readElementText();
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
                if(item.isNew) newBgmCount++;
                item.onAirURL=reader.attributes().value("onAirURL").toString().split(';');
                item.focus=focusSet.contains(item.title);
                QStringList onAirURL(reader.attributes().value("onAirURL").toString().split(';'));
                QStringList sites;
                for(const QString &url:onAirURL)
                {
                    QString name(getSiteName(url));
                    if(!name.isEmpty()) sites<<getSiteName(url);
                }
                item.sitesName=sites.join('|');
                bgms<<item;
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
    endResetModel();
    return true;
}

void BgmList::saveLocal()
{
    QFile localFile(GlobalObjects::dataPath+"bgmLocal.xml");
    bool ret=localFile.open(QIODevice::WriteOnly|QIODevice::Text);
    if(!ret) return;
    QXmlStreamWriter writer(&localFile);
    writer.setAutoFormatting(true);
    writer.writeStartDocument();
    writer.writeStartElement("bgmInfo");
    writer.writeStartElement("local");
    writer.writeStartElement("year");
    writer.writeCharacters(QString::number(localYear));
    writer.writeEndElement();
    writer.writeStartElement("month");
    writer.writeCharacters(QString::number(localMonth));
    writer.writeEndElement();
    writer.writeStartElement("version");
    writer.writeCharacters(dataVersion);
    writer.writeEndElement();
    writer.writeStartElement("focus");
    writer.writeCharacters(focusSet.toList().join(';'));
    writer.writeEndElement();
    writer.writeEndElement();//local
    writer.writeStartElement("list");
    for(auto &item :bgms)
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

void BgmWorker::fetchListInfo(int localYear,int localMonth,const QString &dataVersion,bool forceRefresh)
{
    try
    {
        QJsonObject archive(Network::toJson(Network::httpGet("https://bgmlist.com/tempapi/archive.json",QUrlQuery())).object());
        archive=archive.value("data").toObject();
        auto endIter=--archive.end();
        int nYear = endIter.key().toInt();
        int nMonth = (--endIter->toObject().end()).key().toInt();
        QJsonObject endMObj((--endIter->toObject().end())->toObject());
        QString nDataVersion(endMObj.value("version").toString());
        QString bgmListPath(endMObj.value("path").toString());
        if(nYear!=localYear || nMonth !=localMonth || nDataVersion!=dataVersion || forceRefresh)
        {
            QString content(Network::httpGet(bgmListPath,QUrlQuery()));
            emit fetchDone("",content,nYear,nMonth,nDataVersion);
        }
        emit fetchDone("","",nYear,nMonth,nDataVersion);
    }
    catch (Network::NetworkError &err)
    {
        emit fetchDone(err.errorInfo,"",localYear,localMonth,dataVersion);
    }
}

