TARGET = $$PWD/../lib/dpdf

DEFINES += __QT__ \
    OPJ_STATIC \
    PNG_PREFIX \
    PNG_USE_READ_MACROS \
    BUILD_DEEPIN_PDFIUM_LIB

TEMPLATE = lib

CONFIG += c++11 plugin

EXAMPLE_FILES = dpdfiumcontrol.json

QT = core-private core gui

include($$PWD/3rdparty/pdfium/pdfium.pri)

HEADERS += \
    $$PWD/dpdfiumglobal.h \
    $$PWD/dpdfium.h \
    $$PWD/dpdfiumpage.h \
    dannotation.h \
    dpdfiumcontrolinterface.h \
    dpdfiumcontroller.h \
    dpdfiumdefines.h

SOURCES += \
    $$PWD/dpdfiumglobal.cpp \
    $$PWD/dpdfium.cpp \
    $$PWD/dpdfiumpage.cpp \
    dannotation.cpp \
    dpdfiumcontroller.cpp
