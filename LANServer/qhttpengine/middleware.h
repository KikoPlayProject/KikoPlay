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

#ifndef QHTTPENGINE_MIDDLEWARE_H
#define QHTTPENGINE_MIDDLEWARE_H

#include <QObject>

#include "qhttpengine_export.h"

namespace QHttpEngine
{

class Socket;

/**
 * @brief Pre-handler request processor
 *
 * Middleware sits between the server and the final request handler,
 * determining whether the request should be passed on to the handler.
 */
class QHTTPENGINE_EXPORT Middleware : public QObject
{
    Q_OBJECT

public:

    /**
     * @brief Base constructor for middleware
     */
    explicit Middleware(QObject *parent = Q_NULLPTR) : QObject(parent) {}

    /**
     * @brief Determine if request processing should continue
     *
     * This method is invoked when a new request comes in. If true is
     * returned, processing continues. Otherwise, it is assumed that an
     * appropriate error was written to the socket.
     */
    virtual bool process(Socket *socket) = 0;
};

}

#endif // QHTTPENGINE_MIDDLEWARE_H
