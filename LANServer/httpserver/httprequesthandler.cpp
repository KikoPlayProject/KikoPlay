/**
  @file
  @author Stefan Frings
*/

#include "httprequesthandler.h"

using namespace stefanfrings;

HttpRequestHandler::HttpRequestHandler(QObject* parent)
    : QObject(parent)
{}

HttpRequestHandler::~HttpRequestHandler()
{}

void HttpRequestHandler::service(HttpRequest& request, HttpResponse& response)
{
    Logger::logger()->log(Logger::LANServer, "HttpRequestHandler: you need to override the service() function");
    Logger::logger()->log(Logger::LANServer, "HttpRequestHandler: request=%s %s %s",request.getMethod().data(),request.getPath().data(),request.getVersion().data());
    response.setStatus(501,"not implemented");
    response.write("501 not implemented",true);
}
