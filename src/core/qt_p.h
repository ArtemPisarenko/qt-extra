/**
 * @file    qt_p.h
 * @author  Artem Pisarenko
 * @date    26.03.2018
 * @brief   Private definitions for generic Qt extra macros
 */

#ifndef QTEXTRA_QT_P_H
#define QTEXTRA_QT_P_H

#include <qglobal.h>
#if (QT_VERSION < QT_VERSION_CHECK(5, 7, 0))
# error "Qt version 5.7 or later required"
#endif

#define QTEXTRA_ARG(c) QTEXTRA_ARG_ ## c
#define QTEXTRA_ARG_qarg(type, value) Q_ARG(type, value)
#define QTEXTRA_ARG_t(type, value) type

#define QTEXTRA_RET_ARG(c) QTEXTRA_RET_ARG_ ## c
#define QTEXTRA_RET_ARG_qarg(type, value) Q_RETURN_ARG(type, value)
#define QTEXTRA_RET_ARG_t(type, value) type

#define QTEXTRA_CLASSTYPE(obj) std::remove_reference<decltype(*(obj))>::type
#define QTEXTRA_CLASSMEMBERREF(obj, member) & QTEXTRA_CLASSTYPE(obj) ::member

#endif // QTEXTRA_QT_P_H
