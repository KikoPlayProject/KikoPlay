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

#ifndef QHTTPENGINE_HANDLER_H
#define QHTTPENGINE_HANDLER_H

#include <QObject>

#include "qhttpengine_export.h"

class QRegExp;

namespace QHttpEngine
{

class Middleware;
class Socket;

class QHTTPENGINE_EXPORT HandlerPrivate;

/**
 * @brief Base class for HTTP handlers
 *
 * When a request is received by a [Server](@ref QHttpEngine::Server), it
 * invokes the route() method of the root handler which is used to determine
 * what happens to the request. All HTTP handlers derive from this class and
 * should override the protected process() method in order to process the
 * request. Each handler also maintains a list of redirects and sub-handlers
 * which are used in place of invoking process() when one of the patterns
 * match.
 *
 * To add a redirect, use the addRedirect() method. The first parameter is a
 * QRegExp pattern that the request path will be tested against. If it
 * matches, an HTTP 302 redirect will be written to the socket and the request
 * closed. For example, to have the root path "/" redirect to "/index.html":
 *
 * @code
 * QHttpEngine::Handler handler;
 * handler.addRedirect(QRegExp("^$"), "/index.html");
 * @endcode
 *
 * To add a sub-handler, use the addSubHandler() method. Again, the first
 * parameter is a QRegExp pattern. If the pattern matches, the portion of the
 * path that matched the pattern is removed from the path and it is passed to
 * the sub-handler's route() method. For example, to have a sub-handler
 * invoked when the path begins with "/api/":
 *
 * @code
 * QHttpEngine::Handler handler, subHandler;
 * handler.addSubHandler(QRegExp("^api/"), &subHandler);
 * @endcode
 *
 * If the request doesn't match any redirect or sub-handler patterns, it is
 * passed along to the process() method, which is expected to either process
 * the request or write an error to the socket. The default implementation of
 * process() simply returns an HTTP 404 error.
 */
class QHTTPENGINE_EXPORT Handler : public QObject
{
    Q_OBJECT

public:

    /**
     * @brief Base constructor for a handler
     */
    explicit Handler(QObject *parent = 0);

    /**
     * @brief Add middleware to the handler
     */
    void addMiddleware(Middleware *middleware);

    /**
     * @brief Add a redirect for a specific pattern
     *
     * The pattern and path will be added to an internal list that will be
     * used when the route() method is invoked to determine whether the
     * request matches any patterns. The order of the list is preserved.
     *
     * The destination path may use "%1", "%2", etc. to refer to captured
     * parts of the pattern. The client will receive an HTTP 302 redirect.
     */
    void addRedirect(const QRegExp &pattern, const QString &path);

    /**
     * @brief Add a handler for a specific pattern
     *
     * The pattern and handler will be added to an internal list that will be
     * used when the route() method is invoked to determine whether the
     * request matches any patterns. The order of the list is preserved.
     */
    void addSubHandler(const QRegExp &pattern, Handler *handler);

    /**
     * @brief Route an incoming request
     */
    void route(Socket *socket, const QString &path);

protected:

    /**
     * @brief Process a request
     *
     * This method should process the request either by fulfilling it, sending
     * a redirect with
     * [Socket::writeRedirect()](@ref QHttpEngine::Socket::writeRedirect), or
     * writing an error to the socket using
     * [Socket::writeError()](@ref QHttpEngine::Socket::writeError).
     */
    virtual void process(Socket *socket, const QString &path);

private:

    HandlerPrivate *const d;
    friend class HandlerPrivate;
};

}

#endif // QHTTPENGINE_HANDLER_H
