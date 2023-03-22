#ifndef ROUTER_H
#define ROUTER_H

#include "httpserver/httprequesthandler.h"
class FileHandler;
class APIHandler;
class Router : public stefanfrings::HttpRequestHandler
{
    Q_OBJECT
    Q_DISABLE_COPY(Router)
public:
    Router(QObject* parent = nullptr);
    void service(stefanfrings::HttpRequest& request, stefanfrings::HttpResponse& response);
private:
    FileHandler *fileHandler;
    APIHandler *apiHandler;
};

#endif // ROUTER_H
