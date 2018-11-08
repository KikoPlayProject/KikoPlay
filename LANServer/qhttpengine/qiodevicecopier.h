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

#ifndef QHTTPENGINE_QIODEVICECOPIER_H
#define QHTTPENGINE_QIODEVICECOPIER_H

#include <QObject>

#include "qhttpengine_export.h"

class QIODevice;

namespace QHttpEngine
{

class QHTTPENGINE_EXPORT QIODeviceCopierPrivate;

/**
 * @brief Data copier for classes deriving from QIODevice
 *
 * QIODeviceCopier provides a set of methods for reading data from a QIODevice
 * and writing it to another. The class operates asynchronously and therefore
 * can be used from the main thread. The copier is initialized with pointers
 * to two QIODevices:
 *
 * @code
 * QFile srcFile("src.txt");
 * QFile destFile("dest.txt");
 *
 * QHttpEngine::QIODeviceCopier copier(&srcFile, &destFile);
 * copier.start()
 * @endcode
 *
 * Notice in the example above that it is not necessary to open the devices
 * prior to starting the copy operation. The copier will attempt to open both
 * devices with the appropriate mode if they are not already open.
 *
 * If the source device is sequential, data will be read as it becomes
 * available and immediately written to the destination device. If the source
 * device is not sequential, data will be read and written in blocks. The size
 * of the blocks can be modified with the setBufferSize() method.
 *
 * If an error occurs, the error() signal will be emitted. When the copy
 * completes, either by reading all of the data from the source device or
 * encountering an error, the finished() signal is emitted.
 */
class QHTTPENGINE_EXPORT QIODeviceCopier : public QObject
{
    Q_OBJECT

public:

    /**
     * @brief Create a new device copier from the specified source and destination devices
     */
    QIODeviceCopier(QIODevice *src, QIODevice *dest, QObject *parent = 0);

    /**
     * @brief Set the size of the buffer
     */
    void setBufferSize(qint64 size);

    /**
     * @brief Set range of data to copy, if src device is not sequential
     */
    void setRange(qint64 from, qint64 to);

Q_SIGNALS:

    /**
     * @brief Indicate that an error occurred
     */
    void error(const QString &message);

    /**
     * @brief Indicate that the copy operation finished
     *
     * For sequential devices, this will occur when readChannelFinished() is
     * emitted. For other devices, this signal relies on QIODevice::atEnd()
     * and QIODevice::aboutToClose().
     *
     * This signal will also be emitted immediately after the error() signal
     * or if the stop() method is invoked.
     */
    void finished();

public Q_SLOTS:

    /**
     * @brief Start the copy operation
     *
     * The source device will be opened for reading and the destination device
     * opened for writing if applicable. If opening either file fails for some
     * reason, the error() signal will be emitted.
     *
     * This method should never be invoked more than once.
     */
    void start();

    /**
     * @brief Stop the copy operation
     *
     * The start() method should not be invoked after stopping the operation.
     * Instead, a new QIODeviceCopier instance should be created.
     */
    void stop();

private:

    QIODeviceCopierPrivate *const d;
    friend class QIODeviceCopierPrivate;
};

}

#endif // QHTTPENGINE_QIODEVICECOPIER_H
