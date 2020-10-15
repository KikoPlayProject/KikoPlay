#include "bangumi.h"
#include "Common/network.h"
#include "Common/htmlparsersax.h"

QString Bangumi::animeSearch(const QString &keyword, QList<BangumiInfo> &ret)
{
    QString baseUrl("https://api.bgm.tv/search/subject/"+keyword);
    QUrlQuery query;
    query.addQueryItem("type","2");
    query.addQueryItem("responseGroup","small");
    query.addQueryItem("start","0");
    query.addQueryItem("max_results","25");
    try
    {
        QJsonDocument document(Network::toJson(Network::httpGet(baseUrl,query,QStringList()<<"Accept"<<"application/json")));
        QJsonArray results=document.object().value("list").toArray();
        for(auto iter=results.begin();iter!=results.end();++iter)
        {
            QJsonObject searchObj=(*iter).toObject();
            ret.append({
                           searchObj.value("id").toInt(),
                           searchObj.value("name").toString().replace("&amp;","&"),
                           searchObj.value("name_cn").toString().replace("&amp;","&")
                       });
        }
        return QString();
    }
    catch(Network::NetworkError &error)
    {
        return error.errorInfo;
    }
}

QString Bangumi::getEp(int bgmID, QList<EpInfo> &ret)
{
    QString epUrl(QString("https://api.bgm.tv/subject/%1/ep").arg(bgmID));
    try
    {
        QString str(Network::httpGet(epUrl,QUrlQuery(),QStringList()<<"Accept"<<"application/json"));
        QJsonDocument document(Network::toJson(str));
        QJsonObject obj = document.object();
        QJsonArray epArray=obj.value("eps").toArray();
        for(auto iter=epArray.begin();iter!=epArray.end();++iter)
        {
            QJsonObject epobj=(*iter).toObject();
            ret.append({static_cast<EpInfo::EpType>(epobj.value("type").toInt()),
                        epobj.value("sort").toInt(),
                        epobj.value("name").toString().replace("&amp;","&"),
                        epobj.value("name_cn").toString().replace("&amp;","&")});
        }
        return QString();
    }
    catch(Network::NetworkError &error)
    {
        return error.errorInfo;
    }
}

QString Bangumi::getTags(int bgmID, QStringList &ret)
{
    QString bgmPageUrl(QString("http://bgm.tv/subject/%1").arg(bgmID));
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
                    ret.append(parser.readContentText());
                }
                parser.readNext();
            }
        }
        return QString();
    }
    catch(Network::NetworkError &error)
    {
        return error.errorInfo;
    }
}

QDataStream &operator<<(QDataStream &stream, const Bangumi::EpInfo &ep)
{
    return stream<<static_cast<int>(ep.type)<<ep.index<<ep.name<<ep.name_cn;
}

QDataStream &operator>>(QDataStream &stream, Bangumi::EpInfo &ep)
{
    int type;
    stream>>type;
    ep.type = static_cast<Bangumi::EpInfo::EpType>(type);
    return stream>>ep.index>>ep.name>>ep.name_cn;
}

QDataStream &operator<<(QDataStream &stream, const Bangumi::BangumiInfo &bgm)
{
    return stream<<bgm.bgmID<<bgm.name<<bgm.name_cn;
}

QDataStream &operator>>(QDataStream &stream, Bangumi::BangumiInfo &bgm)
{
    return stream>>bgm.bgmID>>bgm.name>>bgm.name_cn;
}
