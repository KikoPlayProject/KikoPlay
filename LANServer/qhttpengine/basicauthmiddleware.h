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

#ifndef QHTTPENGINE_BASICAUTHMIDDLEWARE_H
#define QHTTPENGINE_BASICAUTHMIDDLEWARE_H

#include <qhttpengine/middleware.h>

#include "qhttpengine_export.h"

namespace QHttpEngine
{

class QHTTPENGINE_EXPORT BasicAuthMiddlewarePrivate;

/**
 * @brief %Middleware for HTTP basic authentication
 *
 * HTTP Basic authentication allows access to specific resources to be
 * restricted. This class uses a map to store accepted username/password
 * combinations, which are then used for authenticating requests. To use a
 * different method of authentication, override the verify() method in a
 * derived class.
 */
class QHTTPENGINE_EXPORT BasicAuthMiddleware : public Middleware
{
    Q_OBJECT

public:

    /**
     * @brief Base constructor for the middleware
     *
     * The realm string is shown to a client when credentials are requested.
     */
    BasicAuthMiddleware(const QString &realm, QObject *parent = Q_NULLPTR);

    /**
     * @brief Add credentials to the list
     *
     * If the username has already been added, its password will be replaced
     * with the new one provided.
     */
    void add(const QString &username, const QString &password);

    /**
     * @brief Process the request
     *
     * If the verify() method returns true, the client will be granted access
     * to the resources. Otherwise, 401 Unauthorized will be returned.
     */
    virtual bool process(Socket *socket);

protected:

    /**
     * @brief Determine if the client is authorized
     */
    virtual bool verify(const QString &username, const QString &password);

private:

    BasicAuthMiddlewarePrivate *const d;
};

}

#endif // QHTTPENGINE_BASICAUTHMIDDLEWARE_H
