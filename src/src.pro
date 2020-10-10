TARGET = $$PWD/../lib/dpdf

TEMPLATE = lib

CONFIG += c++14

QT = core-private core gui

include($$PWD/3rdparty/pdfium/pdfium.pri)

public_headers += \
    $$PWD/dpdfglobal.h \
    $$PWD/dpdfdoc.h \
    $$PWD/dpdfpage.h \
    $$PWD/dpdfannot.h

HEADERS += $$public_headers

SOURCES += \
    $$PWD/dpdfglobal.cpp \
    $$PWD/dpdfdoc.cpp \
    $$PWD/dpdfpage.cpp \
    $$PWD/dpdfannot.cpp

target.path  = /usr/lib

header_files.path = /usr/include

header_files.files = $$public_headers

INSTALLS += target header_files
