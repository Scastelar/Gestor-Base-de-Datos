QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    consultawidget.cpp \
    formulariowidget.cpp \
    gestorformularios.cpp \
    main.cpp \
    mainwindow.cpp \
    querydesignerwidget.cpp \
    relaciondialog.cpp \
    relacionesview.cpp \
    relacioneswidget.cpp \
    relationitem.cpp \
    reportewidget.cpp \
    validadorrelaciones.cpp \
    vistaconsulta.cpp \
    vistadatos.cpp \
    vistadiseno.cpp \
    tableitem.cpp

HEADERS += \
    consultawidget.h \
    formulariowidget.h \
    gestorformularios.h \
    mainwindow.h \
    mainwindow.h \
    metadata.h \
    querydesignerwidget.h \
    relaciondialog.h \
    relacionesview.h \
    relacioneswidget.h \
    relationitem.h \
    reportewidget.h \
    validadorrelaciones.h \
    vistaconsulta.h \
    vistadatos.h \
    vistadiseno.h \
    vistadiseno.h \
    tableitem.h

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

RESOURCES += \
    imgs.qrc
