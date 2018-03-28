#******************************************************************************
# @file    dbus.pri
# @author  Artem Pisarenko
# @date    28.03.2018
# @brief   include file for using "dbus" module in qmake project
#
#******************************************************************************

lessThan(QT_MAJOR_VERSION, 5) | lessThan(QT_MINOR_VERSION, 7): error("Qt version 5.7 or later required")
!contains(QT, dbus) | !contains(QT, network): error("Qt dbus and network modules must be included")

SOURCES += \
    $$PWD/dbus/remotedbusconnection.cpp

HEADERS += \
    $$PWD/dbus/remotedbusconnection.h
