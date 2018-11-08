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

#ifndef QHTTPENGINE_LOCALFILE_H
#define QHTTPENGINE_LOCALFILE_H

#include <QFile>

#include "qhttpengine_export.h"

namespace QHttpEngine
{

class QHTTPENGINE_EXPORT LocalFilePrivate;

/**
 * @brief Locally accessible file
 *
 * LocalFile uses platform-specific functions to create a file containing
 * information that will be accessible only to the local user. This is
 * typically used for storing authentication tokens:
 *
 * @code
 * QHttpEngine::LocalFile file;
 * if (file.open()) {
 *     file.write("private data");
 *     file.close();
 * }
 * @endcode
 *
 * By default, the file is stored in the user's home directory and the name of
 * the file is derived from the value of QCoreApplication::applicationName().
 * For example, if the application name was "test" and the user's home
 * directory was `/home/bob`, the absolute path would be `/home/bob/.test`.
 */
class QHTTPENGINE_EXPORT LocalFile : public QFile
{
    Q_OBJECT

public:

    /**
     * @brief Create a new local file
     */
    explicit LocalFile(QObject *parent = 0);

    /**
     * @brief Attempt to open the file
     *
     * The file must be opened before data can be read or written. This method
     * will return false if the underlying file could not be opened or if this
     * class was unable to set the appropriate file permissions.
     */
    bool open();

private:

    LocalFilePrivate *const d;
    friend class LocalFilePrivate;
};

}

#endif // QHTTPENGINE_LOCALFILE_H
