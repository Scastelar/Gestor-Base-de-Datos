#ifndef BASEDATOSWINDOW_H
#define BASEDATOSWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QTabWidget>
#include <QToolBar>
#include <QToolButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFrame>
#include <QSplitter>
#include <QList>

#include "tablacentralwidget.h"
#include "datasheetwidget.h"
#include "relacioneswidget.h"

class BaseDatosWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit BaseDatosWindow(QWidget *parent = nullptr);


private slots:

    void aplicarEstilos();
    void crearMenus();
    void crearRibbonTabs();
    void crearToolbars();
    void crearRibbonInicio();
    void crearRibbonCrear();

    QFrame* crearSeccionRibbon(const QString &titulo);
    QToolButton* crearBotonRibbon(const QString &iconPath, const QString &texto);

    void mostrarRibbonInicio();
    void mostrarRibbonCrear();

    void abrirTabla(QListWidgetItem *item);
    void cambiarTablaActual(int index);
    void actualizarConexionesBotones();
    void insertarFilaActual();
    void eliminarFilaActual();
    void cambiarVista();

    void abrirRelaciones();
    void cerrarRelacionesYVolver();
    void guardarRelacionEnBD(const QString &tabla1, const QString &campo1,
                                              const QString &tabla2, const QString &campo2);
    void cerrarTab(int index);

    bool nombreTablaEsUnico(const QString &nombreTabla);
    void crearNuevaTabla();
    void guardarTablasAbiertas();

private:
    // Estado
    bool vistaHojaDatos;
    bool filtroActivo;

    // Referencias de tabla actual
    QWidget *tablaActual;
    QString tablaActualNombre;

    // UI principal
    QListWidget *listaTablas;
    QTabWidget *zonaCentral;

    // Ribbon toolbars
    QToolBar *ribbonInicio;
    QToolBar *ribbonCrear;

    // Botones
    QToolButton *btnLlavePrimaria;
    QToolButton *btnFiltrar;
    QToolButton *btnAscendente;
    QToolButton *btnDescendente;
    QToolButton *btnInsertarFila;
    QToolButton *btnEliminarFila;
    QToolButton *btnRelaciones;

    QComboBox *comboVista;
    int encontrarTablaEnTabs(const QString &nombreTabla) const;
    // Layouts
    QVBoxLayout *filasLayout;
    QWidget *botonesFilasWidget;
    QVBoxLayout *botonesFilasVLayout;
    QHBoxLayout *botonesFilasHLayout;

    // Listas de secciones del ribbon
    QList<QFrame*> seccionesVistaHojaDatos;
    QList<QFrame*> seccionesVistaDiseno;
};

#endif // BASEDATOSWINDOW_H
