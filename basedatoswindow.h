// BaseDatosWindow.h
#ifndef BASEDATOSWINDOW_H
#define BASEDATOSWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QMenuBar>
#include <QAction>
#include <QSplitter>
#include <QLabel>

class BaseDatosWindow : public QMainWindow {
    Q_OBJECT
public:
    BaseDatosWindow(QWidget *parent = nullptr);

private slots:
    void abrirTabla(QListWidgetItem *item);

private:
    QListWidget *listaTablas;      // Barra lateral
    QStackedWidget *zonaCentral;   // Donde se muestran tablas o formularios
    QLabel* tituloTablas;
    QLineEdit* barraBusqueda;

    void crearMenus();
};

#endif // BASEDATOSWINDOW_H
