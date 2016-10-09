#-------------------------------------------------
#
# Project created by QtCreator 2016-05-30T22:32:02
#
#-------------------------------------------------

QT       += core gui network multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = shadertoy
TEMPLATE = app

CONFIG += c++11

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
}

include(core.pri)
include(gui.pri)

DISTFILES += \
    .gitignore \
    README.md \
    LICENSE.txt
