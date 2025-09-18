#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
#include "vistaconsulta.h"
#include "vistadiseno.h"
#include "vistadatos.h"
#include "relacioneswidget.h"
#include "querydesignerwidget.h"
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);


private slots:
    void crearNuevaConsulta();
    void aplicarEstilos();
    void crearMenus();
    void crearRibbonTabs();
    void crearToolbars();
    void crearRibbonInicio();
    void crearRibbonCrear();

    void onSolicitarDatosRelacionados(const QString &tabla, const QString &campo, const QString &valor);

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
signals:
    void actualizarRelaciones();
    void metadatosModificados();

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
    void ordenarRegistros(Qt::SortOrder order);
    // Layouts
    QVBoxLayout *filasLayout;
    QWidget *botonesFilasWidget;
    QVBoxLayout *botonesFilasVLayout;
    QHBoxLayout *botonesFilasHLayout;

    // Listas de secciones del ribbon
    QList<QFrame*> seccionesVistaHojaDatos;
    QList<QFrame*> seccionesVistaDiseno;

    void cargarTablaParaEdicion(const QString& nombreTabla);
    QSet<QString> obtenerCamposRelacionados(const QString& nombreTabla);
    void onMetadatosModificados();


};
#endif // MAINWINDOW_H




