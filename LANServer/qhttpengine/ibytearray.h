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

#ifndef QHTTPENGINE_IBYTEARRAY_H
#define QHTTPENGINE_IBYTEARRAY_H

#include <cctype>

#include <QByteArray>

#include "qhttpengine_export.h"

namespace QHttpEngine
{

/**
 * @brief Case-insensitive subclass of QByteArray
 *
 * The IByteArray is identical to the QByteArray class in all aspects except
 * that it performs comparisons in a case-insensitive manner.
 */
class QHTTPENGINE_EXPORT IByteArray : public QByteArray
{
public:

    /// \{
    IByteArray() {}
    IByteArray(const QByteArray &other) : QByteArray(other) {}
    IByteArray(const IByteArray &other) : QByteArray(other) {}
    IByteArray(const char *data, int size = -1) : QByteArray(data, size) {}

    inline bool operator==(const QString &s2) const { return toLower() == s2.toLower(); }
    inline bool operator!=(const QString &s2) const { return toLower() != s2.toLower(); }
    inline bool operator<(const QString &s2) const { return toLower() < s2.toLower(); }
    inline bool operator>(const QString &s2) const { return toLower() > s2.toLower(); }
    inline bool operator<=(const QString &s2) const { return toLower() <= s2.toLower(); }
    inline bool operator>=(const QString &s2) const { return toLower() >= s2.toLower(); }

    bool contains(char c) const { return toLower().contains(tolower(c)); }
    bool contains(const char *c) const { return toLower().contains(QByteArray(c).toLower()); }
    bool contains(const QByteArray &a) const { return toLower().contains(a.toLower()); }
    /// \}
};

inline bool operator==(const IByteArray &a1, const char *a2) { return a1.toLower() == QByteArray(a2).toLower(); }
inline bool operator==(const char *a1, const IByteArray &a2) { return QByteArray(a1).toLower() == a2.toLower(); }
inline bool operator==(const IByteArray &a1, const QByteArray &a2) { return a1.toLower() == a2.toLower(); }
inline bool operator==(const QByteArray &a1, const IByteArray &a2) { return a1.toLower() == a2.toLower(); }
inline bool operator==(const IByteArray &a1, const IByteArray &a2) { return a1.toLower() == a2.toLower(); }

inline bool operator!=(const IByteArray &a1, const char *a2) { return a1.toLower() != QByteArray(a2).toLower(); }
inline bool operator!=(const char *a1, const IByteArray &a2) { return QByteArray(a1).toLower() != a2.toLower(); }
inline bool operator!=(const IByteArray &a1, const QByteArray &a2) { return a1.toLower() != a2.toLower(); }
inline bool operator!=(const QByteArray &a1, const IByteArray &a2) { return a1.toLower() != a2.toLower(); }
inline bool operator!=(const IByteArray &a1, const IByteArray &a2) { return a1.toLower() != a2.toLower(); }

inline bool operator<(const IByteArray &a1, const char *a2) { return a1.toLower() < QByteArray(a2).toLower(); }
inline bool operator<(const char *a1, const IByteArray &a2) { return QByteArray(a1).toLower() < a2.toLower(); }
inline bool operator<(const IByteArray &a1, const QByteArray &a2) { return a1.toLower() < a2.toLower(); }
inline bool operator<(const QByteArray &a1, const IByteArray &a2) { return a1.toLower() < a2.toLower(); }
inline bool operator<(const IByteArray &a1, const IByteArray &a2) { return a1.toLower() < a2.toLower(); }

inline bool operator>(const IByteArray &a1, const char *a2) { return a1.toLower() > QByteArray(a2).toLower(); }
inline bool operator>(const char *a1, const IByteArray &a2) { return QByteArray(a1).toLower() > a2.toLower(); }
inline bool operator>(const IByteArray &a1, const QByteArray &a2) { return a1.toLower() > a2.toLower(); }
inline bool operator>(const QByteArray &a1, const IByteArray &a2) { return a1.toLower() > a2.toLower(); }
inline bool operator>(const IByteArray &a1, const IByteArray &a2) { return a1.toLower() > a2.toLower(); }

inline bool operator<=(const IByteArray &a1, const char *a2) { return a1.toLower() <= QByteArray(a2).toLower(); }
inline bool operator<=(const char *a1, const IByteArray &a2) { return QByteArray(a1).toLower() <= a2.toLower(); }
inline bool operator<=(const IByteArray &a1, const QByteArray &a2) { return a1.toLower() <= a2.toLower(); }
inline bool operator<=(const QByteArray &a1, const IByteArray &a2) { return a1.toLower() <= a2.toLower(); }
inline bool operator<=(const IByteArray &a1, const IByteArray &a2) { return a1.toLower() <= a2.toLower(); }

inline bool operator>=(const IByteArray &a1, const char *a2) { return a1.toLower() >= QByteArray(a2).toLower(); }
inline bool operator>=(const char *a1, const IByteArray &a2) { return QByteArray(a1).toLower() >= a2.toLower(); }
inline bool operator>=(const IByteArray &a1, const QByteArray &a2) { return a1.toLower() >= a2.toLower(); }
inline bool operator>=(const QByteArray &a1, const IByteArray &a2) { return a1.toLower() >= a2.toLower(); }
inline bool operator>=(const IByteArray &a1, const IByteArray &a2) { return a1.toLower() >= a2.toLower(); }

}

#endif // QHTTPENGINE_IBYTEARRAY_H
