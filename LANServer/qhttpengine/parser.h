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

#ifndef QHTTPENGINE_PARSER_H
#define QHTTPENGINE_PARSER_H

#include <QList>

#include <qhttpengine/socket.h>

#include "qhttpengine_export.h"

namespace QHttpEngine
{

/**
 * @brief Utility methods for parsing HTTP requests and responses
 *
 * This class provides a set of static methods for parsing HTTP request and
 * response headers. Functionality is broken up into smaller methods in order
 * to make the unit tests simpler.
 */
class QHTTPENGINE_EXPORT Parser
{
public:

    /**
     * @brief Split a QByteArray by the provided delimiter
     *
     * If the delimiter is not present in the QByteArray, the resulting list
     * will contain the original QByteArray as its only element. The delimiter
     * must not be empty.
     *
     * If maxSplit is nonzero, the list will contain no more than maxSplit + 1
     * items. If maxSplit is equal to zero, there will be no limit on the
     * number of splits performed.
     */
    static void split(const QByteArray &data, const QByteArray &delim, int maxSplit, QByteArrayList &parts);

    /**
     * @brief Parse and remove the query string from a path
     */
    static bool parsePath(const QByteArray &rawPath, QString &path, Socket::QueryStringMap &queryString);

    /**
     * @brief Parse a list of lines containing HTTP headers
     *
     * Each line is expected to be in the format "name: value". Parsing is
     * immediately aborted if an invalid line is encountered.
     */
    static bool parseHeaderList(const QList<QByteArray> &lines, Socket::HeaderMap &headers);

    /**
     * @brief Parse HTTP headers
     *
     * The specified header data (everything up to the double CRLF) is parsed
     * into a status line and HTTP headers. The parts list will contain the
     * parts from the status line.
     */
    static bool parseHeaders(const QByteArray &data, QList<QByteArray> &parts, Socket::HeaderMap &headers);

    /**
     * @brief Parse HTTP request headers
     */
    static bool parseRequestHeaders(const QByteArray &data, Socket::Method &method, QByteArray &path, Socket::HeaderMap &headers);

    /**
     * @brief Parse HTTP response headers
     */
    static bool parseResponseHeaders(const QByteArray &data, int &statusCode, QByteArray &statusReason, Socket::HeaderMap &headers);
};

}

#endif // QHTTPENGINE_PARSER_H
