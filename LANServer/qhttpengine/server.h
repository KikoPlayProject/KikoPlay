/*
 * Copyright (c) 2017 Nathan Osman
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef QHTTPENGINE_SERVER_H
#define QHTTPENGINE_SERVER_H

#include <QHostAddress>
#include <QObject>
#include <QTcpServer>

#include "qhttpengine_export.h"

#if !defined(QT_NO_SSL)
class QSslConfiguration;
#endif

namespace QHttpEngine
{

class Handler;

class QHTTPENGINE_EXPORT ServerPrivate;

/**
 * @brief TCP server for HTTP requests
 *
 * This class provides a TCP server that listens for HTTP requests on the
 * specified address and port. When a new request is received, a
 * [Socket](@ref QHttpEngine::Socket) is created for the QTcpSocket which
 * abstracts a TCP server socket. Once the request headers are received, the
 * root handler is invoked and the request processed. The server assumes
 * ownership of the QTcpSocket.
 *
 * Because [Server](@ref QHttpEngine::Server) derives from QTcpServer,
 * instructing the server to listen on an available port is as simple as
 * invoking listen() with no parameters:
 *
 * @code
 * QHttpEngine::Server server;
 * if (!server.listen()) {
 *     // error handling
 * }
 * @endcode
 *
 * Before passing the socket to the handler, the QTcpSocket's disconnected()
 * signal is connected to the [Socket](@ref QHttpEngine::Socket)'s
 * deleteLater() slot to ensure that the socket is deleted when the client
 * disconnects.
 */
class QHTTPENGINE_EXPORT Server : public QTcpServer
{
    Q_OBJECT

public:

    /**
     * @brief Create an HTTP server
     */
    explicit Server(QObject *parent = 0);

    /**
     * @brief Create an HTTP server with the specified handler
     */
    Server(Handler *handler, QObject *parent = 0);

    /**
     * @brief Set the root handler for all new requests
     */
    void setHandler(Handler *handler);

#if !defined(QT_NO_SSL)
    /**
     * @brief Set the SSL configuration for the server
     *
     * If the configuration is not NULL, the server will begin negotiating
     * connections using SSL/TLS.
     */
    void setSslConfiguration(const QSslConfiguration &configuration);
#endif

protected:

    /**
     * @brief Implementation of QTcpServer::incomingConnection()
     */
    void incomingConnection(qintptr socketDescriptor);

private:

    ServerPrivate *const d;
    friend class ServerPrivate;
};

}

#endif // QHTTPENGINE_SERVER_H
