QT += core gui quick quickcontrols2 testlib
CONFIG += c++17 testcase
TARGET = multiwersum_tests
TEMPLATE = app

INCLUDEPATH += ../src

HEADERS += ../src/backend.h ../src/theme.h ../src/translator.h
SOURCES += multiwersum_tests.cpp ../src/backend.cpp ../src/theme.cpp ../src/translator.cpp
RESOURCES += ../src/resources.qrc
