#-------------------------------------------------
#
# Project created by QtCreator 2013-02-05T23:52:32
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = Tests
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    stylelisttest.cpp

unix: QMAKE_CXXFLAGS += -std=c++0x
