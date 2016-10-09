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

SOURCES += main.cpp\
    log.cpp \
    ShadertoyShaderInput.cpp \
    FramebufferObject.cpp \
    LogView.cpp \
    MainWindow.cpp \
    RenderpassView.cpp \
    Settings.cpp \
    ShaderListModel.cpp \
    ShaderSortModel.cpp \
    ShadertoyApi.cpp \
    ShadertoyRenderer.cpp \
    ShadertoyRenderWidget.cpp \
    ShadertoyShader.cpp \
    TablePlotView.cpp \
    ShadertoyRenderPass.cpp

HEADERS  += \
    log.h \
    FramebufferObject.h \
    LogView.h \
    MainWindow.h \
    RenderpassView.h \
    Settings.h \
    ShaderListModel.h \
    ShaderSortModel.h \
    ShadertoyApi.h \
    ShadertoyRenderer.h \
    ShadertoyRenderwidget.h \
    ShadertoyShader.h \
    TablePlotView.h
