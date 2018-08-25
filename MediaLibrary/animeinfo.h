#ifndef ANIMEINFO_H
#define ANIMEINFO_H
#include <QtCore>
#include <QPixmap>
struct Character
{
    QString name;
    QString name_cn;
    QString actor;
    int bangumiID;
    QByteArray image;
};
struct Episode
{
    QString name;
    QString localFile;
    QString lastPlayTime;
};
struct Anime
{
    QString title;
    int epCount;
    QString summary;
    QString date;
    QPixmap coverPixmap;
    int bangumiID;
    qint64 addTime;
    //QMap<QString,QString> staff;
    QList<QPair<QString,QString> > staff;
    QByteArray cover;
    QList<Episode> eps;
    QList<Character> characters;
};
#endif // ANIMEINFO_H
