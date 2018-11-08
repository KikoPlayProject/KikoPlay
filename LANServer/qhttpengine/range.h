/*
 * Copyright (c) 2017 Aleksei Ermakov
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

#ifndef QHTTPENGINE_RANGE_H
#define QHTTPENGINE_RANGE_H

#include <QString>

#include "qhttpengine_export.h"

namespace QHttpEngine
{

class QHTTPENGINE_EXPORT RangePrivate;

/**
 * @brief HTTP range representation
 *
 * This class provides a representation of HTTP range, described in RFC 7233
 * and used when partial content is requested by the client. When an object is
 * created, optional dataSize can be specified, so that relative ranges can
 * be represented as absolute.
 *
 * @code
 * QHttpEngine::Range range(10, -1, 90);
 * range.from();   //  10
 * range.to();     //  89
 * range.length(); //  80
 *
 * range = QHttpEngine::Range("-500", 1000);
 * range.from();   // 500
 * range.to();     // 999
 * range.length(); // 500
 *
 * range = QHttpEngine::Range(0, -1);
 * range.from();   //   0
 * range.to();     //  -1
 * range.length(); //  -1
 *
 * range = QHttpEngine::Range(range, 100);
 * range.from();   //   0
 * range.to();     //  99
 * range.length(); // 100
 * @endcode
 *
 */
class QHTTPENGINE_EXPORT Range
{
public:

    /**
     * @brief Create a new range
     *
     * An empty Range is considered invalid.
     */
    Range();

    /**
     * @brief Construct a range from the provided string
     *
     * Parses string representation range and constructs new Range. For raw
     * header "Range: bytes=0-100" only "0-100" should be passed to
     * constructor. dataSize may be supplied so that relative ranges could be
     * represented as absolute values.
     */
    Range(const QString &range, qint64 dataSize = -1);

    /**
     * @brief Construct a range from the provided offsets
     *
     * Initialises a new Range with from and to values. dataSize may be
     * supplied so that relative ranges could be represented as absolute
     * values.
     */
    Range(qint64 from, qint64 to, qint64 dataSize = -1);

    /**
     * @brief Construct a range from the another range's offsets
     *
     * Initialises a new  Range with from and to values of other Range.
     * Supplied dataSize is used instead of other dataSize.
     */
    Range(const Range &other, qint64 dataSize);

    /**
     * @brief Destroy the range
     */
    ~Range();

    /**
     * @brief Assignment operator
     */
    Range& operator=(const Range &other);

    /**
     * @brief Retrieve starting position of range
     *
     * If range is set as 'last N bytes' and dataSize is not set, returns -N.
     */
    qint64 from() const;

    /**
     * @brief Retrieve ending position of range
     *
     * If range is set as 'last N bytes' and dataSize is not set, returns -1.
     * If ending position is not set, and dataSize is not set, returns -1.
     */
    qint64 to() const;

    /**
     * @brief Retrieve length of range
     *
     * If ending position is not set, and dataSize is not set, and range is
     * not set as 'last N bytes', returns -1. If range is invalid, returns -1.
     */
    qint64 length() const;

    /**
     * @brief Retrieve dataSize of range
     *
     * If dataSize is not set, this method returns -1.
     */
    qint64 dataSize() const;

    /**
     * @brief Checks if range is valid
     *
     * Range is considered invalid if it is out of bounds, that is when this
     * inequality is false - (from <= to < dataSize).
     *
     * When QHttpRange(const QString&) fails to parse range string, resulting
     * range is also considered invalid.
     *
     * @code
     * QHttpEngine::Range range(1, 0, -1);
     * range.isValid(); // false
     *
     * range = QHttpEngine::Range(512, 1024);
     * range.isValid(); // true
     *
     * range = QHttpEngine::Range("-");
     * range.isValid(); // false
     *
     * range = QHttpEngine::Range("abccbf");
     * range.isValid(); // false
     *
     * range = QHttpEngine::Range(0, 512, 128);
     * range.isValid(); // false
     *
     * range = QHttpEngine::Range(128, 64, 512);
     * range.isValid(); // false
     * @endcode
     */
    bool isValid() const;

    /**
     * @brief Retrieve representation suitable for Content-Range header
     *
     * @code
     * QHttpEngine::Range range(0, 100, 1000);
     * range.contentRange(); // "0-100/1000"
     *
     * // When resource size is unknown
     * range = QHttpEngine::Range(512, 1024);
     * range.contentRange(); // "512-1024/*"
     *
     * // if range request was bad, return resource size
     * range = QHttpEngine::Range(1, 0, 1200);
     * range.contentRange(); // "*\/1200"
     * @endcode
     */
    QString contentRange() const;

private:

    RangePrivate *const d;
};

}

#endif // QHTTPENGINE_RANGE_H
