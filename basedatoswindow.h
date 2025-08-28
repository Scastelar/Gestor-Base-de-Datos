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

class BaseDatosWindow : public QMainWindow
{
    Q_OBJECT

public:
    BaseDatosWindow(QWidget *parent = nullptr);

private slots:
    void abrirTabla(QListWidgetItem *item);
    void toggleFiltro();
    void mostrarRibbonInicio();
    void mostrarRibbonCrear();
    void abrirRelaciones();
    void cerrarRelacionesYVolver();

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

    QListWidget *listaTablas;
    QStackedWidget *zonaCentral;

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

    QVBoxLayout *filasLayout;
    QWidget *botonesFilasWidget;
    QVBoxLayout *botonesFilasVLayout;
    QHBoxLayout *botonesFilasHLayout;

    QList<QFrame*> seccionesVistaHojaDatos;
    QList<QFrame*> seccionesVistaDiseno;

    TablaCentralWidget *tablaDesignActual = nullptr;
    DataSheetWidget *tablaDataSheetActual = nullptr;
};

#endif // BASEDATOSWINDOW_H
