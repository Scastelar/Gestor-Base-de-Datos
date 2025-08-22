#include "basedatoswindow.h"
#include "tablacentralwidget.h"

#include <QScreen>
#include <QGuiApplication>
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QApplication>
#include <QStyle>
#include <QIcon>
#include <QToolBar>
#include <QLineEdit>

BaseDatosWindow::BaseDatosWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QIcon iconTabla(":/imgs/table.png");
    QIcon icon(":/imgs/access.png");
    this->setWindowIcon(icon);

    crearMenus();

    //Barra lateral
    listaTablas = new QListWidget();
    listaTablas->addItem(new QListWidgetItem(iconTabla,"Tabla 1"));
    listaTablas->setIconSize(QSize(20,20));

    connect(listaTablas, &QListWidget::itemClicked, this, &BaseDatosWindow::abrirTabla);

    QVBoxLayout *layoutIzq = new QVBoxLayout;
    QLabel *titulo = new QLabel("Tablas");
    QLineEdit *busqueda = new QLineEdit;
    busqueda->setPlaceholderText("Buscar...");

    connect(busqueda, &QLineEdit::textChanged, this, [=](const QString &texto){
        for (int i = 0; i < listaTablas->count(); i++) {
            QListWidgetItem *item = listaTablas->item(i);
            item->setHidden(!item->text().contains(texto, Qt::CaseInsensitive));
        }
    });

    layoutIzq->addWidget(titulo);
    layoutIzq->addWidget(busqueda);
    layoutIzq->addWidget(listaTablas);

    //Zona central
    zonaCentral = new QStackedWidget();
    zonaCentral->addWidget(new QLabel("Selecciona una tabla de la izquierda"));

    QWidget *contenedorIzq = new QWidget;
    contenedorIzq->setLayout(layoutIzq);

    //dividir lateral y central
    QSplitter *splitter = new QSplitter();
    splitter->addWidget(contenedorIzq);
    splitter->addWidget(zonaCentral);
    splitter->setStretchFactor(1, 3);

    setCentralWidget(splitter);
    resize(QGuiApplication::primaryScreen()->availableSize() * 0.8); // 80% pantalla
    setMinimumSize(900, 600);

    menuBar()->setStyleSheet(
        "QMenuBar { background-color: #ab301b; "
        "           color: white; }"
        "QMenuBar::item { background: transparent; padding: 4px 10px; }"
        "QMenuBar::item:selected { background: #ab791b; }"
        );
}

void BaseDatosWindow::crearMenus() {
    QMenu *menuArchivo = menuBar()->addMenu("Archivo");
    menuArchivo->addAction("Nueva tabla");
    menuArchivo->addAction("Abrir base de datos");
    menuArchivo->addAction("Guardar");
    menuArchivo->addAction("Salir");

    QMenu *menuEditar = menuBar()->addMenu("Editar");
    menuEditar->addAction("Copiar");
    menuEditar->addAction("Pegar");

    QToolBar *toolBar = addToolBar("Edición");
    QAction *agregarFila = toolBar->addAction("Agregar Fila");
    QAction *borrarFila  = toolBar->addAction("Eliminar Fila");

}

void BaseDatosWindow::abrirTabla(QListWidgetItem *item) {
    QWidget *tablaDesign = new TablaCentralWidget();
    zonaCentral->addWidget(tablaDesign);
    zonaCentral->setCurrentWidget(tablaDesign);
}

