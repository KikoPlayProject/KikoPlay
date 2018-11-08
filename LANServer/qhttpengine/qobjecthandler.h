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

#ifndef QHTTPENGINE_QOBJECTHANDLER_H
#define QHTTPENGINE_QOBJECTHANDLER_H

#include <qhttpengine/handler.h>

#include "qhttpengine_export.h"

namespace QHttpEngine
{

class Socket;

class QHTTPENGINE_EXPORT QObjectHandlerPrivate;

/**
 * @brief %Handler for invoking slots
 *
 * This handler enables incoming requests to be processed by slots in a
 * QObject-derived class or functor. Methods are registered by providing a
 * name and slot to invoke. The slot must take a pointer to the
 * [Socket](@ref QHttpEngine::Socket) for the request as an argument and
 * must also close the socket when finished with it.
 *
 * To use this class, simply create an instance and call the appropriate
 * registerMethod() overload. For example:
 *
 * @code
 * class Object : public QObject
 * {
 *     Q_OBJECT
 * public slots:
 *     void something(QHttpEngine::Socket *socket);
 * };
 *
 * QHttpEngine::QObjectHandler handler;
 * Object object;
 * // Old connection syntax
 * handler.registerMethod("something", &object, SLOT(something(QHttpEngine::Socket*)));
 * // New connection syntax
 * handler.registerMethod("something", &object, &Object::something);
 * @endcode
 *
 * It is also possible to use this class with a functor, eliminating the need
 * to create a class and slot:
 *
 * @code
 * QHttpEngine::QObjectHandler handler;
 * handler.registerMethod("something", [](QHttpEngine::Socket *socket) {
 *     // do something
 *     socket->close();
 * });
 * @endcode
 */
class QHTTPENGINE_EXPORT QObjectHandler : public Handler
{
    Q_OBJECT

public:

    /**
     * @brief Create a new QObject handler
     */
    explicit QObjectHandler(QObject *parent = 0);

    /**
     * @brief Register a method
     *
     * This overload uses the traditional connection syntax with macros.
     *
     * The readAll parameter determines whether all data must be received by
     * the socket before invoking the slot.
     */
    void registerMethod(const QString &name, QObject *receiver, const char *method, bool readAll = true);

#ifdef DOXYGEN
    /**
     * @brief Register a method
     *
     * This overload uses the new connection syntax with member pointers.
     */
    void registerMethod(const QString &name, QObject *receiver, PointerToMemberFunction method, bool readAll = true);

    /**
     * @brief Register a method
     *
     * This overload uses the new functor syntax (without context).
     */
    void registerMethod(const QString &name, Functor functor, bool readAll = true);

    /**
     * @brief Register a method
     *
     * This overload uses the new functor syntax (with context).
     */
    void registerMethod(const QString &name, QObject *receiver, Functor functor, bool readAll = true);
#else
    template <typename Func1>
    inline void registerMethod(const QString &name,
                               typename QtPrivate::FunctionPointer<Func1>::Object *receiver,
                               Func1 slot,
                               bool readAll = true) {

        typedef QtPrivate::FunctionPointer<Func1> SlotType;

        // Ensure the slot doesn't have too many arguments
        Q_STATIC_ASSERT_X(int(SlotType::ArgumentCount) == 1,
                          "The slot must have exactly one argument.");

        // Ensure the argument is of the correct type
        Q_STATIC_ASSERT_X((QtPrivate::AreArgumentsCompatible<Socket*, typename QtPrivate::List_Select<typename SlotType::Arguments, 0>::Value>::value),
                          "The slot parameters do not match");

        // Invoke the implementation
        registerMethodImpl(name, receiver, new QtPrivate::QSlotObject<Func1, typename SlotType::Arguments, void>(slot), readAll);
    }

    template <typename Func1>
    inline typename QtPrivate::QEnableIf<!QtPrivate::AreArgumentsCompatible<Func1, QObject*>::value, void>::Type
            registerMethod(const QString &name, Func1 slot, bool readAll = true) {
        registerMethod(name, Q_NULLPTR, slot, readAll);
    }

    template <typename Func1>
    inline typename QtPrivate::QEnableIf<!QtPrivate::FunctionPointer<Func1>::IsPointerToMemberFunction &&
#if QT_VERSION >= QT_VERSION_CHECK(5, 7, 0)
                                             !std::is_same<const char*, Func1>::value,
#else
                                             !QtPrivate::is_same<const char*, Func1>::value,
#endif
                                         void>::Type
            registerMethod(const QString &name, QObject *context, Func1 slot, bool readAll = true) {

        // There is an easier way to do this but then the header wouldn't
        // compile on non-C++11 compilers
        return registerMethod_functor(name, context, slot, &Func1::operator(), readAll);
    }
#endif

protected:

    /**
     * @brief Reimplementation of [Handler::process()](QHttpEngine::Handler::process)
     */
    virtual void process(Socket *socket, const QString &path);

private:

    template <typename Func1, typename Func1Operator>
    inline void registerMethod_functor(const QString &name, QObject *context, Func1 slot, Func1Operator, bool readAll) {

        typedef QtPrivate::FunctionPointer<Func1Operator> SlotType;

        // Ensure the slot doesn't have too many arguments
        Q_STATIC_ASSERT_X(int(SlotType::ArgumentCount) == 1,
                          "The slot must have exactly one argument.");

        // Ensure the argument is of the correct type
        Q_STATIC_ASSERT_X((QtPrivate::AreArgumentsCompatible<Socket*, typename QtPrivate::List_Select<typename SlotType::Arguments, 0>::Value>::value),
                          "The slot parameters do not match");

        registerMethodImpl(name, context,
                           new QtPrivate::QFunctorSlotObject<Func1, 1, typename SlotType::Arguments, void>(slot),
                           readAll);
    }

    void registerMethodImpl(const QString &name, QObject *receiver, QtPrivate::QSlotObjectBase *slotObj, bool readAll);

    QObjectHandlerPrivate *const d;
    friend class QObjectHandlerPrivate;
};

}

#endif // QHTTPENGINE_QOBJECTHANDLER_H
