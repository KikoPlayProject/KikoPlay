#include "lanserver.h"
#include "globalobjects.h"
#include <QSettings>
#include <QThread>
#include <QCoreApplication>
#include "httpserver/httplistener.h"
#include "router.h"

LANServer::LANServer(QObject *parent) : QObject(parent)
{
    httpThread=new QThread();
    httpThread->setObjectName(QStringLiteral("httpThread"));
    httpThread->start(QThread::NormalPriority);

    serverSettings = new QSettings("KikoPlayProject", "KikoPlay");
    serverSettings->setValue("minThreads","4");
    serverSettings->setValue("maxThreads","100");
    serverSettings->setValue("cleanupInterval","60000");
    serverSettings->setValue("readTimeout","60000");
    serverSettings->setValue("maxRequestSize","16000");
    serverSettings->setValue("maxMultiPartSize","10000000");

    router = new Router;
    listener = new stefanfrings::HttpListener(serverSettings, router);
    listener->moveToThread(httpThread);

    bool start=GlobalObjects::appSetting->value("Server/AutoStart",false).toBool();
    if(start)
    {
        quint16 port=GlobalObjects::appSetting->value("Server/Port",8000).toUInt();
        if(!startServer(port))
        {
            GlobalObjects::appSetting->setValue("Server/AutoStart",false);
        }
    }
}

LANServer::~LANServer()
{
    httpThread->quit();
    httpThread->wait();
    listener->deleteLater();
    router->deleteLater();
    serverSettings->deleteLater();
}

bool LANServer::startServer(quint16 port)
{
    serverSettings->setValue("port", port);
    bool ret = true;
    QMetaObject::invokeMethod(listener,"listen",Qt::BlockingQueuedConnection, Q_RETURN_ARG(bool,ret));
    return ret;
}

void LANServer::stopServer()
{
    QMetaObject::invokeMethod(listener,&stefanfrings::HttpListener::close, Qt::BlockingQueuedConnection);
}

bool LANServer::isStart() const
{
    return listener->isListening();
}
