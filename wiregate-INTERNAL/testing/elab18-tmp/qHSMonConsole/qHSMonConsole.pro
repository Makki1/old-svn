# -------------------------------------------------
# Project created by QtCreator 2010-01-15T23:06:10
# -------------------------------------------------
QT += network \
    sql \
    xml
QT -= gui
TARGET = qHSMonConsole
CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app
SOURCES += main.cpp \
    httpget.cpp
HEADERS += httpget.h
