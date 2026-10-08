#-------------------------------------------------
#
# Project created by QtCreator 2014-09-18T17:36:31
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = AmbientTemperature
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

#-------------------------------------------------
# This section will include QextSerialPort in
# your project:

include($$PWD/../../third_party/qextserialport/src/qextserialport.pri)

# The library is bundled in third_party/qextserialport,
# so no separate download is needed.
#-------------------------------------------------