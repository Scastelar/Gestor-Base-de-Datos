#ifndef BASEDATOSWINDOW_H
#define BASEDATOSWINDOW_H

#include "tablacentralwidget.h"
#include "DataSheetWidget.h"

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QMenu>
#include <QToolBar>
#include <QSplitter>
#include <QToolButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget> // Añadir include

class BaseDatosWindow : public QMainWindow
{
    Q_OBJECT

public:
    BaseDatosWindow(QWidget *parent = nullptr);
     ~BaseDatosWindow();

private slots:
    void abrirTabla(QListWidgetItem *item);
    void toggleFiltro();
    void mostrarRibbonInicio();
    void mostrarRibbonCrear();
    void abrirRelaciones();
    void cerrarRelacionesYVolver();
    void cerrarTab(int index); // Nuevo slot para cerrar tabs
    void crearNuevaTabla();

private:
    void crearMenus();
    void crearToolbars();
    void actualizarToolbar(int pestana);
    void crearToolbarHome();
    void crearToolbarCrear();
    void crearToolbarHojaDatos();
    void limpiarToolbars();
    void cambiarVista();
    void transferirDatosVista(QWidget *origen, QWidget *destino);
    int encontrarTablaEnTabs(const QString &nombreTabla); // Nuevo método

    QString tablaActualNombre;

    QListWidget *listaTablas;
    QTabWidget *zonaCentral; // Cambiado de QStackedWidget a QTabWidget

    QToolButton *btnLlavePrimaria;

    // Toolbars
    QToolBar* toolbarInicio;
    QToolBar *toolbarHome;
    QToolBar *toolbarCrear;
    QToolBar *toolbarHojaDatos;
    QToolBar *ribbonInicio;
    QToolBar *ribbonCrear;

    // Acciones comunes
    QAction *accionVista;
    QAction *accionFiltro;

    // Estado
    bool vistaHojaDatos = false;
    bool filtroActivo;

    QComboBox *comboVista;
    QToolButton *btnFiltrar;
    QToolButton *btnAscendente;
    QToolButton *btnDescendente;
    QToolButton *btnInsertarFila;
    QToolButton *btnEliminarFila;
    QToolButton *btnRelaciones;

    QHBoxLayout *filasLayout;
    QWidget *botonesFilasWidget;
    QVBoxLayout *botonesFilasVLayout;
    QHBoxLayout *botonesFilasHLayout;

    QList<QFrame*> seccionesVistaHojaDatos;
    QList<QFrame*> seccionesVistaDiseno;

};

#endif // BASEDATOSWINDOW_H
