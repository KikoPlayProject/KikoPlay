/**
  @file
  @author Stefan Frings
*/

#include "httplistener.h"
#include "httpconnectionhandler.h"
#include "httpconnectionhandlerpool.h"
#include <QCoreApplication>
#include "Common/logger.h"

using namespace stefanfrings;

HttpListener::HttpListener(const QSettings* settings, HttpRequestHandler* requestHandler, QObject *parent)
    : QTcpServer(parent)
{
    Q_ASSERT(settings!=nullptr);
    Q_ASSERT(requestHandler!=nullptr);
    pool=nullptr;
    this->settings=settings;
    this->requestHandler=requestHandler;
    // Reqister type of socketDescriptor for signal/slot handling
    qRegisterMetaType<tSocketDescriptor>("tSocketDescriptor");
    // Start listening
    // listen();
}


HttpListener::~HttpListener()
{
    close();
    Logger::logger()->log(Logger::LANServer, "HttpListener: destroyed");
}


bool HttpListener::listen()
{
    if (isListening()) return true;
    if (!pool)
    {
        pool=new HttpConnectionHandlerPool(settings,requestHandler);
    }
    QString host = settings->value("host").toString();
    quint16 port = settings->value("port").toUInt() & 0xFFFF;
    bool r = QTcpServer::listen(host.isEmpty() ? QHostAddress::AnyIPv4 : QHostAddress(host), port);
    Logger::logger()->log(Logger::LANServer, r?"Server start" : errorString());
    return r;
}


void HttpListener::close() {
    QTcpServer::close();
    Logger::logger()->log(Logger::LANServer, "HttpListener: closed");
    if (pool) {
        delete pool;
        pool=nullptr;
    }
}

void HttpListener::incomingConnection(tSocketDescriptor socketDescriptor) {
#ifdef SUPERVERBOSE
   Logger::logger()->log(Logger::LANServer, "HttpListener: New connection");
#endif

    HttpConnectionHandler* freeHandler=nullptr;
    if (pool)
    {
        freeHandler=pool->getConnectionHandler();
    }

    // Let the handler process the new connection.
    if (freeHandler)
    {
        // The descriptor is passed via event queue because the handler lives in another thread
        QMetaObject::invokeMethod(freeHandler, "handleConnection", Qt::QueuedConnection, Q_ARG(tSocketDescriptor, socketDescriptor));
    }
    else
    {
        // Reject the connection
        Logger::logger()->log(Logger::LANServer, "HttpListener: Too many incoming connections");
        QTcpSocket* socket=new QTcpSocket(this);
        socket->setSocketDescriptor(socketDescriptor);
        connect(socket, SIGNAL(disconnected()), socket, SLOT(deleteLater()));
        socket->write("HTTP/1.1 503 too many connections\r\nConnection: close\r\n\r\nToo many connections\r\n");
        socket->disconnectFromHost();
    }
}
