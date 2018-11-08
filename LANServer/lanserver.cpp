#include "lanserver.h"
#include "httpserver.h"
#include "globalobjects.h"
#include <QSettings>
LANServer::LANServer(QObject *parent) : QObject(parent)
{
    QObject *workObj=new QObject();
    workObj->moveToThread(GlobalObjects::workThread);
    QMetaObject::invokeMethod(workObj,[workObj,this](){
        server=new HttpServer();
        workObj->deleteLater();
    },Qt::BlockingQueuedConnection);
    QObject::connect(server,&HttpServer::showLog,this,[this](const QString &log){
        if(logs.size()>99)logs.pop_front();
        logs.append(log);
        emit showLog(log);
    },Qt::QueuedConnection);
    bool start=GlobalObjects::appSetting->value("Server/AutoStart",false).toBool();
    if(start)
    {
        qint64 port=GlobalObjects::appSetting->value("Server/Port",8000).toLongLong();
        if(!startServer(port).isEmpty())
        {
            GlobalObjects::appSetting->setValue("Server/AutoStart",false);
        }
    }

}

QString LANServer::startServer(qint64 port)
{
    QString retVal;
    QMetaObject::invokeMethod(server,"startServer",Qt::BlockingQueuedConnection,
                              Q_RETURN_ARG(QString,retVal),Q_ARG(qint64,port));
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
