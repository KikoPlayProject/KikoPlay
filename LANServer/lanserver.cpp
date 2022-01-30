#include "lanserver.h"
#include "httpserver.h"
#include "globalobjects.h"
#include <QSettings>
#include <QThread>
LANServer::LANServer(QObject *parent) : QObject(parent)
{
    httpThread=new QThread();
    httpThread->setObjectName(QStringLiteral("httpThread"));
    httpThread->start(QThread::NormalPriority);
    server=new HttpServer();
    server->moveToThread(httpThread);
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
    server->deleteLater();
}

bool LANServer::startServer(quint16 port)
{
    bool ret;
    QMetaObject::invokeMethod(server,"startServer",Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(bool,ret),Q_ARG(quint16,port));
    return ret;
}

void LANServer::stopServer()
{
    QMetaObject::invokeMethod(server,&HttpServer::stopServer,Qt::BlockingQueuedConnection);
}

bool LANServer::isStart() const
{
    return server->isListening();
}
