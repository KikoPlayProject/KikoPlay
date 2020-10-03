#ifndef BANGUMI_H
#define BANGUMI_H
#include<QList>
#include<QString>

class Bangumi
{
public:
    struct BangumiInfo
    {
        int bgmID;
        QString name, name_cn;
    };
    struct EpInfo
    {
        int index;
        QString name, name_cn;
    };
    static QString animeSearch(const QString &keyword, QList<BangumiInfo> &ret);
    static QString getEp(int bgmID, QList<EpInfo> &ret);
    static QString getTags(int bgmID, QStringList &ret);
};

#endif // BANGUMI_H
