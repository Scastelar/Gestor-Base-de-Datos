QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    basedatoswindow.cpp \
    datasheetwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    relaciondialog.cpp \
    relacionesview.cpp \
    relacioneswidget.cpp \
    relationitem.cpp \
    tablacentralwidget.cpp \
    tableitem.cpp

HEADERS += \
    basedatoswindow.h \
    datasheetwidget.h \
    mainwindow.h \
    metadata.h \
    relaciondialog.h \
    relacionesview.h \
    relacioneswidget.h \
    relationitem.h \
    tablacentralwidget.h \
    tableitem.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    imgs.qrc
