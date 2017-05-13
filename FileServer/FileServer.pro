#-------------------------------------------------
#
# Project created by QtCreator 2017-05-03T15:42:35
#
#-------------------------------------------------

QT       += core gui network
RC_ICONS +=Icon.ico
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FileServer
TEMPLATE = app


SOURCES += main.cpp\
        MainWindow.cpp \
    Dialog.cpp

HEADERS  += MainWindow.h \
    Dialog.h

FORMS    += MainWindow.ui \
    Dialog.ui
