#ifndef APIHANDLER_H
#define APIHANDLER_H

#include "httpserver/httprequesthandler.h"
#include <QString>
#include <QJsonDocument>
#include <QHash>

class APIHandler : public stefanfrings::HttpRequestHandler
{
    Q_OBJECT
    Q_DISABLE_COPY(APIHandler)
public:
    APIHandler(QObject* parent = nullptr);
    void service(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
private:
    void apiPlaylist(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiPlaystate(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiUpdateTime(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiDanmu(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiDanmuFull(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiLocalDanmu(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiUpdateDelay(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiUpdateTimeline(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiSubtitle(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiScreenshot(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
    void apiLaunch(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
private:
    QJsonDocument playlistDoc;
    bool readJson(const QByteArray &bytes, QJsonDocument &doc);
};

#endif // APIHANDLER_H
