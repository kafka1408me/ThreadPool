TEMPLATE = app
CONFIG += c++14

QT += core

SOURCES += \
        ThreadPool.cpp \
        main.cpp

HEADERS += \
  ThreadPool.h

DEFINES += DEBUG_POOL
