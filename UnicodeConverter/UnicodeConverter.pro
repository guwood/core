#-------------------------------------------------
#
# Project created by QtCreator 2015-05-15T12:43:02
#
#-------------------------------------------------

QT       -= core gui

VERSION = 1.0.0.1
TARGET = UnicodeConverter
TEMPLATE = lib

#CONFIG += staticlib
CONFIG += shared
CONFIG += c++11

QMAKE_CXXFLAGS += -fvisibility=hidden
QMAKE_CFLAGS += -fvisibility=hidden

QMAKE_LFLAGS += -Wl,--rpath=./

############### destination path ###############
DESTINATION_SDK_PATH = $$PWD/../SDK/lib
ICU_BUILDS_PLATFORM = mac

# WINDOWS
win32:contains(QMAKE_TARGET.arch, x86_64):{
CONFIG(debug, debug|release) {
    DESTDIR = $$DESTINATION_SDK_PATH/win_64/DEBUG
} else {
    DESTDIR = $$DESTINATION_SDK_PATH/win_64
}
ICU_BUILDS_PLATFORM = win64
}
win32:!contains(QMAKE_TARGET.arch, x86_64):{
CONFIG(debug, debug|release) {
    DESTDIR = $$DESTINATION_SDK_PATH/win_32/DEBUG
} else {
    DESTDIR = $$DESTINATION_SDK_PATH/win_32
}
ICU_BUILDS_PLATFORM = win32
}

linux-g++ | linux-g++-64 | linux-g++-32:contains(QMAKE_HOST.arch, x86_64):{
    DESTDIR = $$DESTINATION_SDK_PATH/linux_64
    ICU_BUILDS_PLATFORM = linux64
}
linux-g++ | linux-g++-64 | linux-g++-32:!contains(QMAKE_HOST.arch, x86_64):{
    DESTDIR = $$DESTINATION_SDK_PATH/linux_32
    ICU_BUILDS_PLATFORM = linux32
}

################################################

############# dynamic dependencies #############
shared {
    DEFINES += UNICODECONVERTER_USE_DYNAMIC_LIBRARY
}
################################################

linux-g++ | linux-g++-64 | linux-g++-32 {
    CONFIG += plugin
    TARGET_EXT = .so

    INCLUDEPATH += $$PWD/icubuilds/$$ICU_BUILDS_PLATFORM/usr/local/include
    LIBS        += $$PWD/icubuilds/$$ICU_BUILDS_PLATFORM/usr/local/lib/libicuuc.so.55
    LIBS        += $$PWD/icubuilds/$$ICU_BUILDS_PLATFORM/usr/local/lib/libicudata.so.55
    message(linux)
}

win32 {
    QMAKE_CXXFLAGS_RELEASE -= -Zc:strictStrings
    TARGET_EXT = .dll

    INCLUDEPATH += $$PWD/icubuilds/$$ICU_BUILDS_PLATFORM/include
    LIBS        += -L$$PWD/icubuilds/$$ICU_BUILDS_PLATFORM/lib -licuuc
    message(windows)
}

SOURCES += \
    UnicodeConverter.cpp

HEADERS +=\
    UnicodeConverter.h
