#-------------------------------------------------
#
# Project created by QtCreator 2016-05-30T22:32:02
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = shadertoy
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    shadertoyapi.cpp \
    shadertoyshader.cpp \
    shadertoyrenderer.cpp \
    shaderlistmodel.cpp \
    settings.cpp \
    shadersortmodel.cpp \
    renderpassview.cpp \
    shadertoyrenderwidget.cpp \
    log.cpp \
    logview.cpp

HEADERS  += mainwindow.h \
    shadertoyapi.h \
    shadertoyshader.h \
    shadertoyrenderer.h \
    shaderlistmodel.h \
    settings.h \
    shadersortmodel.h \
    renderpassview.h \
    shadertoyrenderwidget.h \
    log.h \
    logview.h
