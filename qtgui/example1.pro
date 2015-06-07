#-------------------------------------------------
#
# Project created by QtCreator 2014-02-15T16:14:41
#
#-------------------------------------------------

QT       += core gui

CONFIG += static

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = example1
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    tablemodel.cpp \
    appman.cpp

HEADERS  += mainwindow.h \
    tablemodel.h \
    appman.h

FORMS    +=

RESOURCES += \
    resources.qrc
