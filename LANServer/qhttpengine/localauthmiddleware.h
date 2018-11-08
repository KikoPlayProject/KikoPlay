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

#ifndef QHTTPENGINE_LOCALAUTHMIDDLEWARE_H
#define QHTTPENGINE_LOCALAUTHMIDDLEWARE_H

#include <QVariantMap>

#include <qhttpengine/middleware.h>

#include "qhttpengine_export.h"

namespace QHttpEngine
{

class QHTTPENGINE_EXPORT LocalAuthMiddlewarePrivate;

/**
 * @brief %Middleware for local file-based authentication
 *
 * This class is intended for authenticating applications running under the
 * same user account as the server. [LocalFile](@ref QHttpEngine::LocalFile)
 * is used to expose a token to connecting applications. The client passes the
 * token in a special header and the request is permitted.
 *
 * The file consists of a JSON object in the following format:
 *
 * @code
 * {
 *     "token": "{8a34d0f0-29d0-4e54-b3aa-ce8f8ad65527}"
 * }
 * @endcode
 *
 * Additional data can be added to the object using the setData() method.
 */
class QHTTPENGINE_EXPORT LocalAuthMiddleware : public Middleware
{
    Q_OBJECT

public:

    /**
     * @brief Initialize local authentication
     *
     * To determine whether the local file was created successfully, call the
     * exists() method.
     */
    explicit LocalAuthMiddleware(QObject *parent = Q_NULLPTR);

    /**
     * @brief Determine whether the file exists
     */
    bool exists() const;

    /**
     * @brief Retrieve the name of the file used for storing the token
     */
    QString filename() const;

    /**
     * @brief Set additional data to include with the token
     */
    void setData(const QVariantMap &data);

    /**
     * @brief Set the name of the custom header used for confirming the token
     *
     * The default value is "X-Auth-Token".
     */
    void setHeaderName(const QByteArray &name);

    /**
     * @brief Process the request
     *
     * If the token supplied by the client matches, the request is allowed.
     * Otherwise, an HTTP 403 error is returned.
     */
    virtual bool process(Socket *socket);

private:

    LocalAuthMiddlewarePrivate *const d;
};

}

#endif // QHTTPENGINE_LOCALAUTHMIDDLEWARE_H
