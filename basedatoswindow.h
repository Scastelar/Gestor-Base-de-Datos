#ifndef BASEDATOSWINDOW_H
#define BASEDATOSWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QStackedWidget>
#include <QMenu>
#include <QToolBar>
#include <QSplitter>
#include <QToolButton>

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

private:
    void crearMenus();
    void crearToolbars();
    void actualizarToolbar(int pestana);
    void crearToolbarHome();
    void crearToolbarCrear();
    void crearToolbarHojaDatos();
    void limpiarToolbars();

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
    bool vistaHojaDatos;
    bool filtroActivo;
};

#endif // BASEDATOSWINDOW_H
