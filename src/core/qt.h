/**
 * @file    qt.h
 * @author  Artem Pisarenko
 * @date    25.03.2018
 * @brief   Generic Qt extra macros
 */

#ifndef QTEXTRA_QT_H
#define QTEXTRA_QT_H

#include "qt_p.h"
#include <qmetaobject.h>

/**
 * \defgroup QTMETAMETHOD_INVOKE QMetaObject::invokeMethod wrappers
 *
 * Set of QTMETAMETHOD_INVOKE_* macros allows invokation QObject's methods declared as invokable
 * (i.e. signals, slots or suffixed with Q_INVOKABLE).
 * These macros are wrappers around QMetaObject::invokeMethod() with following improvements:
 * - full compile-time check;
 * - slightly reduced reduced invokation code length (omitting Q_ARGs).
 *
 * Unfortunalely, preprocessor doesn't support C++ overload semantics and it wasn't possible
 * to make use of variadic arguments feature.
 * That's why there are separate macro sets for every combination of arguments count and overload variants.
 * Although, not all combinations supported, only most useful.
 *
 * Parameters for macros match exactly same names and order as in QMetaObject::invokeMethod definitions.
 *
 * QTMETAMETHOD_INVOKE_RET_ARGS<count>(obj, member, type, ret, ...)
 *  - is most generic form of invokation supporting all parameters and <count> arguments
 * QTMETAMETHOD_INVOKE_RET(obj, member, type, ret)
 *  - same as QTMETAMETHOD_INVOKE_RET_ARGS<count>, but without arguments
 * QTMETAMETHOD_INVOKE_ARGS<count>(obj, member, type, ret, ...)
 *  - same as QTMETAMETHOD_INVOKE_RET_ARGS<count>, but without return value
 * QTMETAMETHOD_INVOKE(obj, member, type, ret, ...)
 *  - same as QTMETAMETHOD_INVOKE_ARGS<count>, but without without arguments
 * QTMETAMETHOD_INVOKE_QUEUED[_ARGS<count>](obj, member, ...)
 *  - variants for queued-type connection (most popular use case)
 *
 * These macros doesn't require form user explicit specialization for invoking overloaded variants calls.
 * It's automatically deduced from argument types user provided.
 *
 * Special parameters "ret" and "val<n>" must be passed as pair of type and value in parentheses. Type must match exactly as defined in invokable method.
 * Parameter "member" must contain exactly and only method name without any casting or extra symbols (because it's stringified in macro expansion).
 *
 * Example usage:
 * \code{.cpp}
 * QWidget *widget;
 * bool result;
 * result = widget->close();
 * QTMETAMETHOD_INVOKE_RET(widget, close, Qt::QueuedConnection, (bool, result)); // same as previous, but queued
 * QAbstractSocket *socket;
 * QNetworkProxy proxy;
 * QAuthenticator authenticator;
 * socket->proxyAuthenticationRequired(proxy, &authenticator);
 * QTMETAMETHOD_INVOKE_ARGS2(socket, proxyAuthenticationRequired, Qt::BlockingQueuedConnection, (const QNetworkProxy&, proxy), (QAuthenticator*, &authenticator)); // same as previous, but blocked synchronous call
 * \endcode
 */
/**@{*/


//@{
/** Macro wrappers for invoking QObject methods asynchronously and without return value.
 * Equivalent to code
 * \code{.cpp}
 * QMetaObject::invokeMethod(obj, "member", Qt::QueuedConnection, ...);
 * \endcode
 */

#define QTMETAMETHOD_INVOKE_QUEUED(obj, member) \
    QTMETAMETHOD_INVOKE(obj, member, Qt::QueuedConnection)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS1(obj, member, val0) \
    QTMETAMETHOD_INVOKE_ARGS1(obj, member, Qt::QueuedConnection, val0)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS2(obj, member, val0, val1) \
    QTMETAMETHOD_INVOKE_ARGS2(obj, member, Qt::QueuedConnection, val0, val1)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS3(obj, member, val0, val1, val2) \
    QTMETAMETHOD_INVOKE_ARGS3(obj, member, Qt::QueuedConnection, val0, val1, val2)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS4(obj, member, val0, val1, val2, val3) \
    QTMETAMETHOD_INVOKE_ARGS4(obj, member, Qt::QueuedConnection, val0, val1, val2, val3)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS5(obj, member, val0, val1, val2, val3, val4) \
    QTMETAMETHOD_INVOKE_ARGS5(obj, member, Qt::QueuedConnection, val0, val1, val2, val3, val4)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS6(obj, member, val0, val1, val2, val3, val4, val5) \
    QTMETAMETHOD_INVOKE_ARGS6(obj, member, Qt::QueuedConnection, val0, val1, val2, val3, val4, val5)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS7(obj, member, val0, val1, val2, val3, val4, val5, val6) \
    QTMETAMETHOD_INVOKE_ARGS7(obj, member, Qt::QueuedConnection, val0, val1, val2, val3, val4, val5, val6)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS8(obj, member, val0, val1, val2, val3, val4, val5, val6, val7) \
    QTMETAMETHOD_INVOKE_ARGS8(obj, member, Qt::QueuedConnection, val0, val1, val2, val3, val4, val5, val6, val7)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS9(obj, member, val0, val1, val2, val3, val4, val5, val6, val7, val8) \
    QTMETAMETHOD_INVOKE_ARGS9(obj, member, Qt::QueuedConnection, val0, val1, val2, val3, val4, val5, val6, val7, val8)

#define QTMETAMETHOD_INVOKE_QUEUED_ARGS10(obj, member, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9) \
    QTMETAMETHOD_INVOKE_ARGS10(obj, member, Qt::QueuedConnection, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9)

//@}


//@{
/** Macro wrappers for invoking QObject methods without return value.
 * Equivalent to code
 * \code{.cpp}
 * QMetaObject::invokeMethod(obj, "member", type, ...);
 * \endcode
 */

#define QTMETAMETHOD_INVOKE(obj, member, type) do { \
    (void)QOverload<>::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type);\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS1(obj, member, type, val0) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS2(obj, member, type, val0, val1) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS3(obj, member, type, val0, val1, val2) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1, \
                    QTEXTRA_ARG(t) val2 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS4(obj, member, type, val0, val1, val2, val3) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1, \
                    QTEXTRA_ARG(t) val2, \
                    QTEXTRA_ARG(t) val3 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS5(obj, member, type, val0, val1, val2, val3, val4) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1, \
                    QTEXTRA_ARG(t) val2, \
                    QTEXTRA_ARG(t) val3, \
                    QTEXTRA_ARG(t) val4 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS6(obj, member, type, val0, val1, val2, val3, val4, val5) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1, \
                    QTEXTRA_ARG(t) val2, \
                    QTEXTRA_ARG(t) val3, \
                    QTEXTRA_ARG(t) val4, \
                    QTEXTRA_ARG(t) val5 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS7(obj, member, type, val0, val1, val2, val3, val4, val5, val6) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1, \
                    QTEXTRA_ARG(t) val2, \
                    QTEXTRA_ARG(t) val3, \
                    QTEXTRA_ARG(t) val4, \
                    QTEXTRA_ARG(t) val5, \
                    QTEXTRA_ARG(t) val6 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5, \
        QTEXTRA_ARG(qarg) val6 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS8(obj, member, type, val0, val1, val2, val3, val4, val5, val6, val7) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1, \
                    QTEXTRA_ARG(t) val2, \
                    QTEXTRA_ARG(t) val3, \
                    QTEXTRA_ARG(t) val4, \
                    QTEXTRA_ARG(t) val5, \
                    QTEXTRA_ARG(t) val6, \
                    QTEXTRA_ARG(t) val7 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5, \
        QTEXTRA_ARG(qarg) val6, \
        QTEXTRA_ARG(qarg) val7 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS9(obj, member, type, val0, val1, val2, val3, val4, val5, val6, val7, val8) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1, \
                    QTEXTRA_ARG(t) val2, \
                    QTEXTRA_ARG(t) val3, \
                    QTEXTRA_ARG(t) val4, \
                    QTEXTRA_ARG(t) val5, \
                    QTEXTRA_ARG(t) val6, \
                    QTEXTRA_ARG(t) val7, \
                    QTEXTRA_ARG(t) val8 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5, \
        QTEXTRA_ARG(qarg) val6, \
        QTEXTRA_ARG(qarg) val7, \
        QTEXTRA_ARG(qarg) val8 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_ARGS10(obj, member, type, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9) do { \
    (void)QOverload < \
                    QTEXTRA_ARG(t) val0, \
                    QTEXTRA_ARG(t) val1, \
                    QTEXTRA_ARG(t) val2, \
                    QTEXTRA_ARG(t) val3, \
                    QTEXTRA_ARG(t) val4, \
                    QTEXTRA_ARG(t) val5, \
                    QTEXTRA_ARG(t) val6, \
                    QTEXTRA_ARG(t) val7, \
                    QTEXTRA_ARG(t) val8, \
                    QTEXTRA_ARG(t) val9 \
                    >::of(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5, \
        QTEXTRA_ARG(qarg) val6, \
        QTEXTRA_ARG(qarg) val7, \
        QTEXTRA_ARG(qarg) val8, \
        QTEXTRA_ARG(qarg) val9 \
        );\
} while(0)
//@}


//@{
/** Macro wrappers for invoking QObject methods.
 * Equivalent to code
 * \code{.cpp}
 * QMetaObject::invokeMethod(obj, "member", type, ret, ...);
 * \endcode
 */

#define QTMETAMETHOD_INVOKE_RET(obj, member, type, ret) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)(int)>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret);\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS1(obj, member, type, ret, val0) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS2(obj, member, type, ret, val0, val1) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS3(obj, member, type, ret, val0, val1, val2) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1, \
        QTEXTRA_ARG(t) val2 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS4(obj, member, type, ret, val0, val1, val2, val3) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1, \
        QTEXTRA_ARG(t) val2, \
        QTEXTRA_ARG(t) val3 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS5(obj, member, type, ret, val0, val1, val2, val3, val4) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1, \
        QTEXTRA_ARG(t) val2, \
        QTEXTRA_ARG(t) val3, \
        QTEXTRA_ARG(t) val4 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS6(obj, member, type, ret, val0, val1, val2, val3, val4, val5) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1, \
        QTEXTRA_ARG(t) val2, \
        QTEXTRA_ARG(t) val3, \
        QTEXTRA_ARG(t) val4, \
        QTEXTRA_ARG(t) val5 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS7(obj, member, type, ret, val0, val1, val2, val3, val4, val5, val6) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1, \
        QTEXTRA_ARG(t) val2, \
        QTEXTRA_ARG(t) val3, \
        QTEXTRA_ARG(t) val4, \
        QTEXTRA_ARG(t) val5, \
        QTEXTRA_ARG(t) val6 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5, \
        QTEXTRA_ARG(qarg) val6 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS8(obj, member, type, ret, val0, val1, val2, val3, val4, val5, val6, val7) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1, \
        QTEXTRA_ARG(t) val2, \
        QTEXTRA_ARG(t) val3, \
        QTEXTRA_ARG(t) val4, \
        QTEXTRA_ARG(t) val5, \
        QTEXTRA_ARG(t) val6, \
        QTEXTRA_ARG(t) val7 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5, \
        QTEXTRA_ARG(qarg) val6, \
        QTEXTRA_ARG(qarg) val7 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS9(obj, member, type, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1, \
        QTEXTRA_ARG(t) val2, \
        QTEXTRA_ARG(t) val3, \
        QTEXTRA_ARG(t) val4, \
        QTEXTRA_ARG(t) val5, \
        QTEXTRA_ARG(t) val6, \
        QTEXTRA_ARG(t) val7, \
        QTEXTRA_ARG(t) val8 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5, \
        QTEXTRA_ARG(qarg) val6, \
        QTEXTRA_ARG(qarg) val7, \
        QTEXTRA_ARG(qarg) val8 \
        );\
} while(0)

#define QTMETAMETHOD_INVOKE_RET_ARGS10(obj, member, type, ret, val0, val1, val2, val3, val4, val5, val6, val7, val8, val9) do { \
    (void)static_cast<QTEXTRA_RET_ARG(t) ret (QTEXTRA_CLASSTYPE(obj) ::*)( \
        QTEXTRA_ARG(t) val0, \
        QTEXTRA_ARG(t) val1, \
        QTEXTRA_ARG(t) val2, \
        QTEXTRA_ARG(t) val3, \
        QTEXTRA_ARG(t) val4, \
        QTEXTRA_ARG(t) val5, \
        QTEXTRA_ARG(t) val6, \
        QTEXTRA_ARG(t) val7, \
        QTEXTRA_ARG(t) val8, \
        QTEXTRA_ARG(t) val9 \
        )>(QTEXTRA_CLASSMEMBERREF(obj, member));\
    QMetaObject::invokeMethod(obj, #member, type, QTEXTRA_RET_ARG(qarg) ret, \
        QTEXTRA_ARG(qarg) val0, \
        QTEXTRA_ARG(qarg) val1, \
        QTEXTRA_ARG(qarg) val2, \
        QTEXTRA_ARG(qarg) val3, \
        QTEXTRA_ARG(qarg) val4, \
        QTEXTRA_ARG(qarg) val5, \
        QTEXTRA_ARG(qarg) val6, \
        QTEXTRA_ARG(qarg) val7, \
        QTEXTRA_ARG(qarg) val8, \
        QTEXTRA_ARG(qarg) val9 \
        );\
} while(0)

//@}

/**@}*/ // end of QTMETAMETHOD_INVOKE

#endif // QTEXTRA_QT_H
