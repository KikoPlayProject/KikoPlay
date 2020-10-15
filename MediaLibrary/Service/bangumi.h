#ifndef BANGUMI_H
#define BANGUMI_H
#include<QList>
#include<QString>
#include <QObject>
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
        enum class EpType
        {
            EP, SP, OP, ED, Trailer, MAD, Other
        };
        EpType type;
        int index;
        QString name, name_cn;
        QString toString() const
        {
            if(type == EpType::EP || type == EpType::SP)
                return QObject::tr("No.%0 %1").arg(index).arg(name_cn.isEmpty()?name:name_cn);
            return name_cn.isEmpty()?name:name_cn;
        }
    };
    static QString animeSearch(const QString &keyword, QList<BangumiInfo> &ret);
    static QString getEp(int bgmID, QList<EpInfo> &ret);
    static QString getTags(int bgmID, QStringList &ret);
};
QDataStream &operator<<(QDataStream &stream, const Bangumi::BangumiInfo &bgm);
QDataStream &operator>>(QDataStream &stream, Bangumi::BangumiInfo &bgm);

QDataStream &operator<<(QDataStream &stream, const Bangumi::EpInfo &ep);
QDataStream &operator>>(QDataStream &stream, Bangumi::EpInfo &ep);

#endif // BANGUMI_H
