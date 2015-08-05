#-------------------------------------------------
#
# Project created by QtCreator 2015-08-02T15:03:40
#
#-------------------------------------------------

QT       += core gui sql webenginewidgets multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

win32 {
    RC_ICONS = img/Mao_Kun.ico
}

TARGET = TempestAlarm
TEMPLATE = app

include(deps/zlib/zlib.pri)


SOURCES += main.cpp\
        mainwindow.cpp \
    guncompress.cpp \
    restrictivesqltablemodel.cpp \
    customsqlquerymodel.cpp \
    parsereplytask.cpp

HEADERS  += mainwindow.h \
    guncompress.h \
    restrictivesqltablemodel.h \
    customsqlquerymodel.h \
    parsereplytask.h

FORMS    += mainwindow.ui

RESOURCES +=
