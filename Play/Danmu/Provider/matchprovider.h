#ifndef MATCHPROVIDER_H
#define MATCHPROVIDER_H
#include "info.h"
class MatchWorker : public QObject
{
    Q_OBJECT
private:
    void handleMatchReply(QJsonDocument &document, MatchInfo *matchInfo);
    void handleDDSearchReply(QJsonDocument &document, MatchInfo *searchInfo);
public:
    static MatchInfo *retrieveInMatchTable(QString fileHash,const QString cName="MT");
signals:
    void matchDone(MatchInfo *MatchInfo);
    void ddSearchDone(MatchInfo *searchInfo);
    void bgmSearchDone(MatchInfo *searchInfo);
public slots:
    void beginMatch(QString fileName);
    void beginDDSearch(QString keyword);
    void beginBGMSearch(QString keyword);
};
class MatchProvider
{
public:
    static MatchInfo *SearchFormDandan(const QString &keyword);
    static MatchInfo *SearchFormBangumi(const QString &keyword);
    static MatchInfo *SerchFromDB(const QString &keyword);
    static MatchInfo *MatchFromDandan(QString fileName);
    static MatchInfo *MatchFromDB(QString fileName);
    static QString updateMatchInfo(QString fileName,MatchInfo *newMatchInfo,const QString cName="MT");
    static void addToMatchTable(QString fileHash,QString poolID,bool replace=false,const QString cName="MT");
    static QString addToBangumiTable(QString animeTitle,QString title,const QString cName="MT");
private:
    static MatchWorker *matchWorker;
};

#endif // MATCHPROVIDER_H
