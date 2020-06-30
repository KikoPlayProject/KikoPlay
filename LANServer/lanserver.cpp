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
    QObject::connect(server,&HttpServer::showLog,this,[this](const QString &log){
        if(logs.size()>99)logs.pop_front();
        logs.append(log);
        emit showLog(log);
    },Qt::QueuedConnection);
    bool start=GlobalObjects::appSetting->value("Server/AutoStart",false).toBool();
    if(start)
    {
        quint16 port=GlobalObjects::appSetting->value("Server/Port",8000).toUInt();
        if(!startServer(port).isEmpty())
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

QString LANServer::startServer(quint16 port)
{
    QString retVal;
    QMetaObject::invokeMethod(server,"startServer",Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString,retVal),Q_ARG(quint16,port));
    return retVal;
}

void LANServer::stopServer()
{
    QMetaObject::invokeMethod(server,&HttpServer::stopServer,Qt::BlockingQueuedConnection);
}

bool LANServer::isStart() const
{
    return server->isListening();
}
