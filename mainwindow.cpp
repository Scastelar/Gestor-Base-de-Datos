#include "mainwindow.h"
#include "vistadatos.h"
#include "relacioneswidget.h"
#include "metadata.h"
#include "vistadiseno.h"

#include <QDebug>
#include <QInputDialog>
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
#include <QTabWidget>
#include <QComboBox>
#include <QMenuBar>
#include <QMenu>
#include <QActionGroup>
#include <QStackedWidget>
#include <QFrame>
#include <QHBoxLayout>
#include <QWidgetAction>
#include <QDir>
#include <QMessageBox>
#include <QSet>
#include <QTimer>
#include <QPushButton> // Include QPushButton
#include <QToolButton> // Include QToolButton
#include "consultawidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), vistaHojaDatos(false), filtroActivo(false), tablaActual(nullptr),
    listaTablas(nullptr), zonaCentral(nullptr), ribbonInicio(nullptr), ribbonCrear(nullptr),
    btnLlavePrimaria(nullptr), btnFiltrar(nullptr), btnAscendente(nullptr), btnDescendente(nullptr),
    btnInsertarFila(nullptr), btnEliminarFila(nullptr), btnRelaciones(nullptr), comboVista(nullptr),
    filasLayout(nullptr), botonesFilasWidget(nullptr), botonesFilasVLayout(nullptr), botonesFilasHLayout(nullptr)
{
    QIcon iconTabla(":/imgs/datasheet-view.png");
    QIcon icon(":/imgs/access.png");
    this->setWindowIcon(icon);

    // Inicializar layouts para botones de filas
    botonesFilasVLayout = new QVBoxLayout();
    botonesFilasHLayout = new QHBoxLayout();
    botonesFilasVLayout->setSpacing(5);
    botonesFilasVLayout->setContentsMargins(0, 0, 0, 0);
    botonesFilasHLayout->setSpacing(5);
    botonesFilasHLayout->setContentsMargins(0, 0, 0, 0);

    crearMenus();
    crearToolbars();

    // Barra lateral
    listaTablas = new QListWidget();
    listaTablas->setIconSize(QSize(20, 20));

    // Cargar todas las tablas guardadas (.meta)
    QDir dir(QDir::currentPath() + "/tables");
    if (!dir.exists()) {
        dir.mkpath("."); // crear si no existe
    }

    QStringList archivosMeta = dir.entryList(QStringList() << "*.meta", QDir::Files);

    for (const QString &fileName : archivosMeta) {
        try {
            Metadata meta = Metadata::cargar(dir.filePath(fileName));
            if (!meta.nombreTabla.isEmpty()) {
                QListWidgetItem *item = new QListWidgetItem(listaTablas);
                item->setData(Qt::UserRole, meta.nombreTabla); // Store table name
                listaTablas->addItem(item);

                // Create custom widget for the item
                QWidget* itemWidget = new QWidget();
                QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
                QLabel* nameLabel = new QLabel(meta.nombreTabla);
                QToolButton* menuButton = new QToolButton();
                menuButton->setIcon(QIcon(":/imgs/menu.png")); // Use a menu icon
                menuButton->setAutoRaise(true);

                // Create the menu
                QMenu* menu = new QMenu(this);
                QAction* renameAction = menu->addAction("Editar Nombre");
                QAction* deleteAction = menu->addAction("Eliminar");

                // Connect actions to slots
                connect(renameAction, &QAction::triggered, this, [this, meta]() {
                    editarNombreTabla(meta.nombreTabla);
                });
                connect(deleteAction, &QAction::triggered, this, [this, meta]() {
                    eliminarTabla(meta.nombreTabla);
                });
                menuButton->setMenu(menu);
                menuButton->setPopupMode(QToolButton::InstantPopup); // Show menu on click

                itemLayout->addWidget(nameLabel);
                itemLayout->addStretch();
                itemLayout->addWidget(menuButton);
                itemLayout->setContentsMargins(0, 0, 0, 0);

                item->setSizeHint(itemWidget->sizeHint());
                listaTablas->setItemWidget(item, itemWidget);
            }
        } catch (const std::runtime_error& e) {
            QMessageBox::warning(this, "Error de Carga",
                                 QString("No se pudo cargar la tabla desde el archivo '%1'. Error: %2").arg(fileName).arg(e.what()));
        }
    }

    connect(listaTablas, &QListWidget::itemClicked, this, &MainWindow::abrirTabla);

    QVBoxLayout *layoutIzq = new QVBoxLayout;
    QLabel *titulo = new QLabel("TABLAS");
    titulo->setStyleSheet("QLabel { font-weight: bold; font-size: 12px; padding: 8px 5px; color: #2b579a; background-color: #f0f0f0; }");

    QLineEdit *busqueda = new QLineEdit;
    busqueda->setPlaceholderText("Buscar tabla...");
    busqueda->setStyleSheet("QLineEdit { padding: 6px; margin: 5px; border: 1px solid #cccccc; border-radius: 3px; }");

    connect(busqueda, &QLineEdit::textChanged, this, [=](const QString &texto) {
        for (int i = 0; i < listaTablas->count(); i++) {
            QListWidgetItem *item = listaTablas->item(i);
            item->setHidden(!item->text().contains(texto, Qt::CaseInsensitive));
        }
    });

    layoutIzq->addWidget(titulo);
    layoutIzq->addWidget(busqueda);
    layoutIzq->addWidget(listaTablas);
    layoutIzq->setSpacing(0);
    layoutIzq->setContentsMargins(0, 0, 0, 0);

    // Zona central - QTabWidget
    zonaCentral = new QTabWidget();
    zonaCentral->setTabsClosable(true);
    zonaCentral->setMovable(true);

    // Conectar señales
    connect(zonaCentral, &QTabWidget::tabCloseRequested, this, &MainWindow::cerrarTab);
    connect(zonaCentral, &QTabWidget::currentChanged, this, &MainWindow::cambiarTablaActual);

    // Widget de bienvenida inicial
    QLabel *welcomeLabel = new QLabel("<center><h2>Bienvenido a Base de Datos</h2>"
                                      "<p>Selecciona una tabla de la izquierda para comenzar</p></center>");
    welcomeLabel->setAlignment(Qt::AlignCenter);

    QWidget *welcomeWidget = new QWidget();
    QVBoxLayout *welcomeLayout = new QVBoxLayout(welcomeWidget);
    welcomeLayout->addWidget(welcomeLabel);
    welcomeWidget->setLayout(welcomeLayout);

    zonaCentral->addTab(welcomeWidget, "Inicio");
    zonaCentral->setTabEnabled(0, false);

    QWidget *contenedorIzq = new QWidget;
    contenedorIzq->setLayout(layoutIzq);
    contenedorIzq->setMinimumWidth(250);
    contenedorIzq->setStyleSheet("QWidget { background-color: #f8f8f8; border-right: 1px solid #cccccc; }");

    // Dividir lateral y central
    QSplitter *splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(contenedorIzq);
    splitter->addWidget(zonaCentral);
    splitter->setSizes(QList<int>() << 250 << 650);
    splitter->setHandleWidth(1);
    splitter->setStyleSheet("QSplitter::handle { background-color: #cccccc; }");

    setCentralWidget(splitter);
    resize(QGuiApplication::primaryScreen()->availableSize() * 0.8);
    setMinimumSize(1000, 700);

    // Estilos para la aplicación
    aplicarEstilos();

    // Mostrar ribbon inicial (Inicio)
    mostrarRibbonInicio();
    addToolBar(Qt::TopToolBarArea, ribbonInicio);
    ribbonCrear->setVisible(false);

    connect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        vistaHojaDatos = (index == 1);
        cambiarVista();
    });



    // Conectar botones de insertar y eliminar fila
    // connect(btnInsertarFila, &QToolButton::clicked, this, &MainWindow::insertarFilaActual);
    // connect(btnEliminarFila, &QToolButton::clicked, this, &MainWindow::eliminarFilaActual);
}

void MainWindow::aplicarEstilos()
{
    QString styleSheet = R"(
        QMainWindow {
            background-color: #ffffff;
        }
        QMenuBar {
            background-color: #2b579a;
            color: white;
            font-weight: bold;
            padding: 0px;
        }
        QMenuBar::item {
            background: transparent;
            padding: 8px 15px;
        }
        QMenuBar::item:selected {
            background: #1e3f6e;
            border-radius: 0px;
        }
        QMenuBar::item:pressed {
            background: #152c4d;
        }
        QToolBar {
            background-color: #f0f0f0;
            border-bottom: 1px solid #cccccc;
            spacing: 3px;
            padding: 2px;
        }
        QToolButton {
            padding: 6px 8px;
            border: 1px solid transparent;
            border-radius: 3px;
            background: transparent;
        }
        QToolButton:hover {
            background-color: #e0e0e0;
            border: 1px solid #cccccc;
        }
        QToolButton:pressed {
            background-color: #d0d0d0;
        }
        QToolButton:checked {
            background-color: #d0e0f0;
            border: 1px solid #a0c0e0;
        }
        QListWidget {
            background-color: white;
            border: none;
            border-top: 1px solid #cccccc;
        }
        QListWidget::item {
            padding: 8px 5px;
            border-bottom: 1px solid #eeeeee;
            font-size: 11px;
        }
        QListWidget::item:selected {
            background-color: #2b579a;
            color: white;
        }
        QListWidget::item:hover {
            background-color: #f0f0f0;
        }
        .ribbonSection {
            background-color: #ffffff;
            border-right: 1px solid #dddddd;
            padding: 5px;
        }
        .ribbonButton {
            padding: 8px 12px;
            border: 1px solid transparent;
            border-radius: 3px;
            background: transparent;
            text-align: center;
        }
        .ribbonButton:hover {
            background-color: #e0e0e0;
            border: 1px solid #cccccc;
        }
    )";

    menuBar()->setStyleSheet(styleSheet);
    if (ribbonInicio) ribbonInicio->setStyleSheet(styleSheet);
    if (ribbonCrear) ribbonCrear->setStyleSheet(styleSheet);
}

void MainWindow::crearMenus()
{
    QMenu *menuArchivo = menuBar()->addMenu("Archivo");
    menuArchivo->addAction(QIcon(":/imgs/new.png"), "Nueva Base de Datos");
    menuArchivo->addAction(QIcon(":/imgs/open.png"), "Abrir");
    menuArchivo->addAction(QIcon(":/imgs/save.png"), "Guardar");
    menuArchivo->addAction(QIcon(":/imgs/save-as.png"), "Guardar Como");
    menuArchivo->addSeparator();
    menuArchivo->addAction(QIcon(":/imgs/exit.png"), "Salir");

    crearRibbonTabs();

    QMenu *menuAyuda = menuBar()->addMenu("Ayuda");
    menuAyuda->addAction(QIcon(":/imgs/help.png"), "Ayuda");
    menuAyuda->addAction(QIcon(":/imgs/about.png"), "Acerca de");
}

void MainWindow::crearRibbonTabs()
{
    QWidget *ribbonTabWidget = new QWidget();
    QHBoxLayout *ribbonTabLayout = new QHBoxLayout(ribbonTabWidget);
    ribbonTabLayout->setSpacing(0);
    ribbonTabLayout->setContentsMargins(20, 0, 0, 0);

    // Botón Inicio
    QToolButton *btnInicio = new QToolButton();
    btnInicio->setText("Inicio");
    btnInicio->setCheckable(true);
    btnInicio->setChecked(true);
    btnInicio->setStyleSheet(
        "QToolButton {"
        "   background: #2b579a;"
        "   color: white;"
        "   padding: 8px 15px;"
        "   border: none;"
        "   border-right: 1px solid #1e3f6e;"
        "}"
        "QToolButton:checked {"
        "   background: #1e3f6e;"
        "}"
        "QToolButton:hover {"
        "   background: #1e3f6e;"
        "}"
        );
    connect(btnInicio, &QToolButton::clicked, this, &MainWindow::mostrarRibbonInicio);

    // Botón Crear
    QToolButton *btnCrear = new QToolButton();
    btnCrear->setText("Crear");
    btnCrear->setCheckable(true);
    btnCrear->setStyleSheet(
        "QToolButton {"
        "   background: #2b579a;"
        "   color: white;"
        "   padding: 8px 15px;"
        "   border: none;"
        "}"
        "QToolButton:checked {"
        "   background: #1e3f6e;"
        "}"
        "QToolButton:hover {"
        "   background: #1e3f6e;"
        "}"
        );
    connect(btnCrear, &QToolButton::clicked, this, &MainWindow::mostrarRibbonCrear);

    ribbonTabLayout->addWidget(btnInicio);
    ribbonTabLayout->addWidget(btnCrear);
    ribbonTabLayout->addStretch();

    menuBar()->setCornerWidget(ribbonTabWidget, Qt::TopLeftCorner);
}

void MainWindow::crearToolbars()
{
    crearRibbonInicio();
    crearRibbonCrear();
}

void MainWindow::crearRibbonInicio()
{
    ribbonInicio = new QToolBar("Ribbon Inicio", this);
    ribbonInicio->setMovable(false);
    ribbonInicio->setIconSize(QSize(16, 16));

    QWidget *inicioWidget = new QWidget();
    QHBoxLayout *inicioLayout = new QHBoxLayout();
    inicioLayout->setSpacing(10);
    inicioLayout->setContentsMargins(15, 5, 15, 5);

    // Sección Vista
    QFrame *vistaFrame = crearSeccionRibbon("Vista");
    QVBoxLayout *vistaLayout = new QVBoxLayout(vistaFrame);

    comboVista = new QComboBox();
    comboVista->addItem(QIcon(":/imgs/design-view.png"), "Vista Diseño");
    comboVista->addItem(QIcon(":/imgs/datasheet-view.png"), "Vista Hoja de Datos");
    comboVista->setStyleSheet("QComboBox { height: 50px; }");

    vistaLayout->addWidget(comboVista);
    vistaFrame->setLayout(vistaLayout);

    // Sección Filtros
    QFrame *filtrosFrame = crearSeccionRibbon("Filtros");
    QVBoxLayout *filtrosLayout = new QVBoxLayout(filtrosFrame);

    btnFiltrar = crearBotonRibbon(":/imgs/filter.png", "Filtrar");
    filtrosLayout->addWidget(btnFiltrar);
    filtrosFrame->setVisible(false);

    // Sección Orden
    QFrame *ordenFrame = crearSeccionRibbon("Orden");
    QHBoxLayout *ordenLayout = new QHBoxLayout(ordenFrame);

    btnAscendente = crearBotonRibbon(":/imgs/sort-asc.png", "Ascendente");
    btnDescendente = crearBotonRibbon(":/imgs/sort-desc.png", "Descendente");

    ordenLayout->addWidget(btnAscendente);
    ordenLayout->addWidget(btnDescendente);
    ordenFrame->setVisible(false);

    // Conectar los botones de ordenamiento
    connect(btnAscendente, &QToolButton::clicked, this, [this]() {
        ordenarRegistros(Qt::AscendingOrder);
    });
    connect(btnDescendente, &QToolButton::clicked, this, [this]() {
        ordenarRegistros(Qt::DescendingOrder);
    });

    // Separador
    QFrame *separador = new QFrame();
    separador->setFrameShape(QFrame::VLine);
    separador->setFrameShadow(QFrame::Sunken);
    separador->setStyleSheet("background-color: #d0d0d0;");
    separador->setVisible(false);

    // Sección Llave Primaria
    QFrame *primaryKeyFrame = crearSeccionRibbon("Clave");
    QVBoxLayout *primaryKeyLayout = new QVBoxLayout(primaryKeyFrame);

    btnLlavePrimaria = crearBotonRibbon(":/imgs/key.png", "Llave Primaria");
    primaryKeyLayout->addWidget(btnLlavePrimaria);

    // Sección Filas - CORREGIDA
    QFrame *filasFrame = crearSeccionRibbon("Filas");
    QHBoxLayout *filasMainLayout = new QHBoxLayout(filasFrame);

    // Crear los botones
    btnInsertarFila = crearBotonRibbon(":/imgs/insert-row.png", "Insertar Fila");
    btnEliminarFila = crearBotonRibbon(":/imgs/delete-row.png", "Eliminar Fila");

    // Crear widget contenedor y layouts
    botonesFilasWidget = new QWidget();
    //botonesFilasVLayout = new QVBoxLayout();
    botonesFilasHLayout = new QHBoxLayout(botonesFilasWidget);

    // Configurar layouts
    //botonesFilasVLayout->setSpacing(5);
    //botonesFilasVLayout->setContentsMargins(0, 0, 0, 0);
    botonesFilasHLayout->setSpacing(5);
    botonesFilasHLayout->setContentsMargins(0, 0, 0, 0);

    // Agregar botones a ambos layouts
    //botonesFilasVLayout->addWidget(btnInsertarFila);
    //botonesFilasVLayout->addWidget(btnEliminarFila);
    botonesFilasHLayout->addWidget(btnInsertarFila);
    botonesFilasHLayout->addWidget(btnEliminarFila);

    // Usar layout vertical por defecto
    botonesFilasWidget->setLayout(botonesFilasHLayout);
    filasMainLayout->addWidget(botonesFilasWidget);

    // Sección Relaciones
    QFrame *relacionesFrame = crearSeccionRibbon("Relaciones");
    QVBoxLayout *relacionesLayout = new QVBoxLayout(relacionesFrame);

    btnRelaciones = crearBotonRibbon(":/imgs/relationships.png", "Relaciones");
    connect(btnRelaciones, &QToolButton::clicked, this, &MainWindow::abrirRelaciones);
    relacionesLayout->addWidget(btnRelaciones);

    // Agregar secciones
    inicioLayout->addWidget(vistaFrame);
    inicioLayout->addWidget(filtrosFrame);
    inicioLayout->addWidget(ordenFrame);
    inicioLayout->addWidget(separador);
    inicioLayout->addWidget(primaryKeyFrame);
    inicioLayout->addWidget(filasFrame);
    inicioLayout->addWidget(relacionesFrame);
    inicioLayout->addStretch();

    inicioWidget->setLayout(inicioLayout);
    ribbonInicio->addWidget(inicioWidget);

    // Guardar referencias
    seccionesVistaHojaDatos << filtrosFrame << ordenFrame << separador;
    seccionesVistaDiseno << primaryKeyFrame << relacionesFrame;

}

void MainWindow::crearRibbonCrear()
{
    ribbonCrear = new QToolBar("Ribbon Crear", this);
    ribbonCrear->setMovable(false);
    ribbonCrear->setIconSize(QSize(16, 16));

    QWidget *crearWidget = new QWidget();
    QHBoxLayout *crearLayout = new QHBoxLayout();
    crearLayout->setSpacing(10);
    crearLayout->setContentsMargins(15, 5, 15, 5);

    // Sección Tablas
    QFrame *tablasFrame = crearSeccionRibbon("Tablas");
    QHBoxLayout *tablasLayout = new QHBoxLayout(tablasFrame);

    QToolButton *btnTabla = crearBotonRibbon(":/imgs/datasheet-view.png", "Tabla");
    QToolButton *btnDisenoTabla = crearBotonRibbon(":/imgs/form-design.png", "Diseño");
    connect(btnTabla, &QToolButton::clicked, this, &MainWindow::crearNuevaTabla);

    tablasLayout->addWidget(btnTabla);
    tablasLayout->addWidget(btnDisenoTabla);

    // Sección Consultas
    QFrame *queriesFrame = crearSeccionRibbon("Consultas");
    QHBoxLayout *queriesLayout = new QHBoxLayout(queriesFrame);

    QToolButton *btnConsulta = crearBotonRibbon(":/imgs/query.png", "Consulta");
    connect(btnConsulta, &QToolButton::clicked, this, &MainWindow::crearNuevaConsulta);

    QToolButton *btnDisenoConsulta = crearBotonRibbon(":/imgs/form-design.png", "Diseño");

    queriesLayout->addWidget(btnConsulta);
    queriesLayout->addWidget(btnDisenoConsulta);

    // Sección Formularios
    QFrame *formsFrame = crearSeccionRibbon("Formularios");
    QHBoxLayout *formsLayout = new QHBoxLayout(formsFrame);

    QToolButton *btnFormulario = crearBotonRibbon(":/imgs/form.png", "Formulario");
    QToolButton *btnDisenoFormulario = crearBotonRibbon(":/imgs/form-design.png", "Diseño");

    formsLayout->addWidget(btnFormulario);
    formsLayout->addWidget(btnDisenoFormulario);

    // Sección Reportes
    // Sección Reportes
    QFrame *reportsFrame = crearSeccionRibbon("Reportes");
    QHBoxLayout *reportsLayout = new QHBoxLayout(reportsFrame);

    QToolButton *btnReporte = crearBotonRibbon(":/imgs/report.png", "Reporte");
    QToolButton *btnDisenoReporte = crearBotonRibbon(":/imgs/form-design.png", "Diseño");

    reportsLayout->addWidget(btnReporte);
    reportsLayout->addWidget(btnDisenoReporte);

    crearLayout->addWidget(tablasFrame);
    crearLayout->addWidget(queriesFrame);
    crearLayout->addWidget(formsFrame);
    crearLayout->addWidget(reportsFrame);
    crearLayout->addStretch();

    crearWidget->setLayout(crearLayout);
    ribbonCrear->addWidget(crearWidget);

    connect(btnReporte, &QToolButton::clicked, this, [=]() {
        QVector<Metadata> metadatos;

        // 🔹 Cargar todas las tablas disponibles
        QDir dir(QDir::currentPath() + "/tables");
        QStringList archivosMeta = dir.entryList(QStringList() << "*.meta", QDir::Files);
        for (const QString &fileName : archivosMeta) {
            Metadata meta = Metadata::cargar(dir.filePath(fileName));
            metadatos.append(meta);
        }

        // 🔹 Crear y mostrar el reporte
        ReporteWidget *reporte = new ReporteWidget(this);
        reporte->generarReporte(metadatos);
        reporte->exec();
    });


}

QFrame* MainWindow::crearSeccionRibbon(const QString &titulo)
{
    QFrame *frame = new QFrame();
    frame->setFrameStyle(QFrame::Box);
    frame->setStyleSheet("QFrame { background: white; border: 1px solid #d0d0d0; border-radius: 3px; }");
    frame->setMinimumWidth(100);

    QLabel *titleLabel = new QLabel(titulo);
    titleLabel->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; padding: 2px; }");
    titleLabel->setAlignment(Qt::AlignCenter);
    return frame;
}

QToolButton* MainWindow::crearBotonRibbon(const QString &iconPath, const QString &texto)
{
    QToolButton *btn = new QToolButton();
    btn->setIcon(QIcon(iconPath));
    btn->setText(texto);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setStyleSheet(".ribbonButton");
    return btn;
}

void MainWindow::mostrarRibbonInicio()
{
    // Ocultar todos los ribbons primero
    if (ribbonCrear && ribbonCrear->isVisible()) {
        removeToolBar(ribbonCrear);
        ribbonCrear->setVisible(false);
    }

    // Mostrar el ribbon de Inicio si no está visible
    if (ribbonInicio && !ribbonInicio->isVisible()) {
        addToolBar(Qt::TopToolBarArea, ribbonInicio);
        ribbonInicio->setVisible(true);
    }

    // Usar el estado actual del combo box
    bool esHojaDatos = (comboVista && comboVista->currentIndex() == 1);

    // Mostrar/ocultar secciones según la vista
    foreach(QFrame *frame, seccionesVistaHojaDatos) {
        if (frame) frame->setVisible(esHojaDatos);
    }

    foreach(QFrame *frame, seccionesVistaDiseno) {
        if (frame) frame->setVisible(!esHojaDatos);
    }

    if (!esHojaDatos){
            // Actualizar propiedades para la vista diseño si existe una tabla abierta
            QWidget *currentTab = zonaCentral->currentWidget();
            if (currentTab && currentTab != zonaCentral->widget(0)) {
                VistaDiseno *tablaDesign = currentTab->property("tablaDesign").value<VistaDiseno*>();
                if (tablaDesign) {
                    tablaDesign->actualizarPropiedades();
                }
            }
    }
}

void MainWindow::mostrarRibbonCrear()
{
    // Ocultar todos los ribbons primero
    if (ribbonInicio && ribbonInicio->isVisible()) {
        removeToolBar(ribbonInicio);
        ribbonInicio->setVisible(false);
    }

    // Mostrar el ribbon de Crear si no está visible
    if (ribbonCrear && !ribbonCrear->isVisible()) {
        addToolBar(Qt::TopToolBarArea, ribbonCrear);
        ribbonCrear->setVisible(true);
    }

    // Actualizar el estado de los botones
    QWidget *cornerWidget = menuBar()->cornerWidget(Qt::TopLeftCorner);
    if (cornerWidget) {
        QList<QToolButton*> buttons = cornerWidget->findChildren<QToolButton*>();
        if (buttons.size() >= 2) {
            buttons[0]->setChecked(false);
            buttons[1]->setChecked(true);
        }
    }
}

int MainWindow::encontrarTablaEnTabs(const QString &nombreTabla) const
{
    for (int i = 0; i < zonaCentral->count(); ++i) {
        if (zonaCentral->tabText(i) == nombreTabla) {
            return i;
        }
    }
    return -1;
}

void MainWindow::abrirTabla(QListWidgetItem *item)
{
    if (!item) return;

    tablaActualNombre = item->data(Qt::UserRole).toString();
    Metadata meta;

    // Si el nombre está vacío, significa que el item no es una tabla real
    if (tablaActualNombre.isEmpty()) {
        qDebug() << "Intento de abrir un item sin nombre de tabla.";
        return;
    }

    // Ya existe una pestaña abierta para esta tabla?
    for (int i = 0; i < zonaCentral->count(); ++i) {
        if (zonaCentral->tabText(i) == tablaActualNombre) {
            zonaCentral->setCurrentIndex(i);
            return;
        }
    }

    try {
        meta = Metadata::cargar(QDir::currentPath() + "/tables/" + tablaActualNombre + ".meta");
    } catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Error al Abrir Tabla",
                              QString("No se pudo abrir el archivo de la tabla '%1'. Error: %2").arg(tablaActualNombre).arg(e.what()));
        return;
    }

    // Crear un widget contenedor para las vistas de la tabla
    QWidget *tablaContainer = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(tablaContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    // Crear stacked widget para alternar entre vista diseño y hoja de datos
    QStackedWidget *tablaStacked = new QStackedWidget();

    // Crear ambas vistas
    VistaDiseno *tablaDesign = new VistaDiseno();
    VistaDatos *tablaDataSheet = new VistaDatos();

    // ⭐ CRÍTICO: Establecer nombre de tabla ANTES de cargar metadatos
    tablaDataSheet->establecerNombreTabla(tablaActualNombre);

    // Cargar estructura en ambas vistas
    tablaDesign->cargarCampos(meta.campos);
    tablaDataSheet->cargarDesdeMetadata(meta);
    tablaDataSheet->cargarRelaciones("relationships.dat");

    // Añadir ambas vistas al stacked widget
    tablaStacked->addWidget(tablaDesign);
    tablaStacked->addWidget(tablaDataSheet);


    // Obtener campos relacionados de relationships.dat
    QSet<QString> camposRelacionados = obtenerCamposRelacionados(tablaActualNombre);
    tablaDesign->setNombreTabla(tablaActualNombre);
    tablaDesign->setCamposRelacionados(camposRelacionados);

    // Conectar señal de modificación
    connect(tablaDesign, &VistaDiseno::metadatosModificados,
            this, &MainWindow::onMetadatosModificados);

    // Configurar la vista inicial según comboVista
    if (comboVista->currentIndex() == 1) {
        tablaStacked->setCurrentWidget(tablaDataSheet);
    } else {
        tablaStacked->setCurrentWidget(tablaDesign);
    }

    containerLayout->addWidget(tablaStacked);

    // Añadir el nuevo tab
    int newTabIndex = zonaCentral->addTab(tablaContainer, tablaActualNombre);
    zonaCentral->setCurrentIndex(newTabIndex);

    // Guardar referencias dentro del tab
    tablaContainer->setProperty("tablaDesign", QVariant::fromValue(tablaDesign));
    tablaContainer->setProperty("tablaDataSheet", QVariant::fromValue(tablaDataSheet));
    tablaContainer->setProperty("tablaStacked", QVariant::fromValue(tablaStacked));

    // Conectar botón de PK a la vista activa
    //disconnect(btnLlavePrimaria, &QToolButton::clicked, 0, 0);
    actualizarConexionesBotones();
    if (comboVista->currentIndex() == 1) {
        connect(btnLlavePrimaria, &QToolButton::clicked,
                tablaDataSheet, &VistaDatos::establecerPK);
    } else {
        connect(btnLlavePrimaria, &QToolButton::clicked,
                tablaDesign, &VistaDiseno::establecerPK);
    }

    // Mostrar ribbon de Inicio
    mostrarRibbonInicio();
}

void MainWindow::cambiarTablaActual(int index)
{
    if (index == 0 || zonaCentral->count() <= 1) {
        tablaActual = nullptr;
        tablaActualNombre.clear();
        return;
    }

    QString tabName = zonaCentral->tabText(index);
    if (tabName == "Relaciones") {
        RelacionesWidget *relaciones = qobject_cast<RelacionesWidget*>(zonaCentral->widget(index));
        if (relaciones) {
            relaciones->refrescarTablas();  // 🔹 ahora refresca sin perder relaciones
            // En el constructor o donde crees RelacionesWidget
            connect(this, &MainWindow::actualizarRelaciones, this, [this]() {
                // Buscar la pestaña de relaciones y actualizarla
                for (int i = 0; i < zonaCentral->count(); ++i) {
                    RelacionesWidget *relaciones = qobject_cast<RelacionesWidget*>(zonaCentral->widget(i));
                    if (relaciones) {
                        relaciones->refrescarTablas();
                        break;
                    }
                }
            });
        }
        return;
    }


    QWidget *currentTab = zonaCentral->widget(index);
    tablaActual = currentTab;
    tablaActualNombre = tabName;

    actualizarConexionesBotones();
}
//Metodo nuevo
void MainWindow::actualizarTablasAbiertasConRelaciones() {
    qDebug() << "🔄 Actualizando todas las tablas abiertas con nuevas relaciones...";

    for (int i = 0; i < zonaCentral->count(); ++i) {
        QString tabName = zonaCentral->tabText(i);

        // Ignorar pestañas especiales
        if (tabName.startsWith("Diseño de Consulta") ||
            tabName.startsWith("Consulta") ||
            tabName.startsWith("Relaciones") ||
            tabName == "Inicio") {
            continue;
        }

        QWidget *tabContainer = zonaCentral->widget(i);
        if (!tabContainer) continue;

        // Obtener las vistas
        VistaDiseno *tablaDesign = tabContainer->property("tablaDesign").value<VistaDiseno*>();
        VistaDatos *tablaDataSheet = tabContainer->property("tablaDataSheet").value<VistaDatos*>();

        if (tablaDesign) {
            // ⭐ ACTUALIZAR CAMPOS RELACIONADOS EN VISTA DISEÑO
            QSet<QString> camposRelacionados = obtenerCamposRelacionados(tabName);
            tablaDesign->setCamposRelacionados(camposRelacionados);
            tablaDesign->actualizarEstadoCampos();
            qDebug() << "✅ Actualizados campos relacionados para tabla" << tabName << ":" << camposRelacionados;
        }

        if (tablaDataSheet) {
            // ⭐ RECARGAR RELACIONES EN VISTA DATOS
            tablaDataSheet->cargarRelaciones("relationships.dat");
            qDebug() << "✅ Recargadas relaciones para VistaDatos de tabla" << tabName;
        }
    }
    // ⭐ TAMBIÉN ACTUALIZAR LA PESTAÑA DE RELACIONES SI ESTÁ ABIERTA
    for (int i = 0; i < zonaCentral->count(); ++i) {
        if (zonaCentral->tabText(i) == "Relaciones") {
            RelacionesWidget *relacionesWidget = qobject_cast<RelacionesWidget*>(zonaCentral->widget(i));
            if (relacionesWidget) {
                relacionesWidget->refrescarTablas();
                qDebug() << "✅ Pestaña de relaciones actualizada";
            }
            break;
        }
    }
}

void MainWindow::actualizarConexionesBotones()
{
    if (!tablaActual) {
        qDebug() << "⚠️ actualizarConexionesBotones: No hay tabla actual";
        return;
    }

    // Desconectar todas las conexiones previas
    disconnect(btnLlavePrimaria, &QToolButton::clicked, 0, 0);
    disconnect(btnInsertarFila, &QToolButton::clicked, 0, 0);
    disconnect(btnEliminarFila, &QToolButton::clicked, 0, 0);

    // Obtener las vistas de la tabla actual
    VistaDiseno *tablaDesign = tablaActual->property("tablaDesign").value<VistaDiseno*>();
    VistaDatos *tablaDataSheet = tablaActual->property("tablaDataSheet").value<VistaDatos*>();

    if (vistaHojaDatos && tablaDataSheet) {
        qDebug() << "🔗 Conectando botones a VistaDatos para tabla:" << tablaActualNombre;

        // ⭐ VERIFICAR que VistaDatos tiene nombre de tabla configurado
        if (tablaDataSheet->obtenerNombreTabla().isEmpty()) {
            qDebug() << "⚠️ VistaDatos no tiene nombre de tabla, configurando:" << tablaActualNombre;
            tablaDataSheet->establecerNombreTabla(tablaActualNombre);
        }

        connect(btnLlavePrimaria, &QToolButton::clicked, tablaDataSheet, &VistaDatos::establecerPK);
        connect(btnInsertarFila, &QToolButton::clicked, tablaDataSheet, &VistaDatos::agregarRegistro);
        connect(btnEliminarFila, &QToolButton::clicked, tablaDataSheet, &VistaDatos::eliminarRegistro);
        connect(tablaDataSheet, &VistaDatos::solicitarDatosRelacionados,
                this, &MainWindow::onSolicitarDatosRelacionados);
    } else if (tablaDesign) {
        qDebug() << "🔗 Conectando botones a VistaDiseno para tabla:" << tablaActualNombre;
        connect(btnLlavePrimaria, &QToolButton::clicked, tablaDesign, &VistaDiseno::establecerPK);
        connect(btnInsertarFila, &QToolButton::clicked, tablaDesign, &VistaDiseno::agregarCampo);
        connect(btnEliminarFila, &QToolButton::clicked, tablaDesign, &VistaDiseno::eliminarCampo);
    }
}

void MainWindow::insertarFilaActual()
{
    if (!tablaActual) return;

    if (vistaHojaDatos) {
        VistaDatos *tablaDataSheet = tablaActual->property("tablaDataSheet").value<VistaDatos*>();
        if (tablaDataSheet) tablaDataSheet->agregarRegistro();
    } else {
        VistaDiseno *tablaDesign = tablaActual->property("tablaDesign").value<VistaDiseno*>();
        if (tablaDesign) tablaDesign->agregarCampo();
    }
}

void MainWindow::eliminarFilaActual()
{
    if (!tablaActual) return;

    if (vistaHojaDatos) {
        VistaDatos *tablaDataSheet = tablaActual->property("tablaDataSheet").value<VistaDatos*>();
        // if (tablaDataSheet) tablaDataSheet->eliminarRegistro();
    } else {
        VistaDiseno *tablaDesign = tablaActual->property("tablaDesign").value<VistaDiseno*>();
        if (tablaDesign) tablaDesign->eliminarCampo();
    }
}

void MainWindow::onSolicitarDatosRelacionados(const QString &tabla, const QString &campo, const QString &valor)
{
    // Si quieres cargar datos existentes además de permitir edición:
    //QList<QMap<QString, QVariant>> datosExistentes = baseDeDatos.obtenerRelacionesExistentes(tabla, campo, valor);

    // Enviar datos existentes al widget
    // dataSheetWidget->onDatosRelacionadosRecibidos(datosExistentes);

    qDebug() << "Solicitando datos relacionados para:" << tabla << campo << valor;
}


void MainWindow::cambiarVista()
{
    if (!tablaActual) return;

    QString tabName = zonaCentral->tabText(zonaCentral->currentIndex());

    // 🔹 No guardar consultas ni relaciones como tablas
    if (tabName.startsWith("Diseño de Consulta") ||
        tabName.startsWith("Consulta") ||
        tabName.startsWith("Relaciones")) {
        return;
    }

    QStackedWidget *tablaStacked = tablaActual->property("tablaStacked").value<QStackedWidget*>();
    VistaDiseno *tablaDesign = tablaActual->property("tablaDesign").value<VistaDiseno*>();
    VistaDatos *tablaDataSheet = tablaActual->property("tablaDataSheet").value<VistaDatos*>();


    tablaDataSheet->cargarRelaciones("relationships.dat");
    RelacionesWidget *relaciones = tablaActual->property("relaciones").value<RelacionesWidget*>();

    // En el constructor o donde crees RelacionesWidget
    connect(this, &MainWindow::actualizarRelaciones, this, [this]() {
        // Buscar la pestaña de relaciones y actualizarla
        for (int i = 0; i < zonaCentral->count(); ++i) {
            RelacionesWidget *relaciones = qobject_cast<RelacionesWidget*>(zonaCentral->widget(i));
            if (relaciones) {
                relaciones->refrescarTablas();
                break;
            }
        }
    });

    if (!tablaStacked) return;

    // ⭐ CRÍTICO: Asegurar que VistaDatos tiene el nombre de tabla correcto
    if (tablaDataSheet && !tablaActualNombre.isEmpty()) {
        tablaDataSheet->establecerNombreTabla(tablaActualNombre);
        tablaDataSheet->cargarRelaciones("relationships.dat");
    }

    // 🔹 Crear metadata con lo que esté actualmente en memoria
    Metadata meta(tablaActualNombre);

    if (tablaDesign) {
        meta.campos = tablaDesign->obtenerCampos();
    }
    if (tablaDataSheet) {
        meta.registros = tablaDataSheet->obtenerRegistros(meta.campos);
    }

    try {
        meta.guardar();  // guarda estructura + datos en .meta y .dat
    } catch (const std::runtime_error &e) {
        QMessageBox::warning(this, "Error al guardar", e.what());
        return;
    }

    // 🔹 Desconectar combo temporalmente para evitar loops
    disconnect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), 0, 0);

    if (vistaHojaDatos) {
        comboVista->setCurrentIndex(1);

        if (tablaDataSheet) {
            Metadata recargada = Metadata::cargar(QDir::currentPath() + "/tables/" + tablaActualNombre + ".meta");

            // ⭐ IMPORTANTE: Establecer nombre ANTES de cargar
            tablaDataSheet->establecerNombreTabla(tablaActualNombre);
            tablaDataSheet->cargarDesdeMetadata(recargada);
            tablaDataSheet->cargarRelaciones("relationships.dat");

            tablaStacked->setCurrentWidget(tablaDataSheet);
        }
    } else {
        comboVista->setCurrentIndex(0);

        if (tablaDesign) {
            try {
                Metadata recargada = Metadata::cargar(QDir::currentPath() + "/tables/" + tablaActualNombre + ".meta");
                tablaDesign->cargarCampos(recargada.campos);
                tablaDesign->setNombreTabla(tablaActualNombre);

                // ⭐ CRÍTICO: Actualizar campos relacionados DESPUÉS de cargar
                QSet<QString> camposRelacionados = obtenerCamposRelacionados(tablaActualNombre);
                tablaDesign->setCamposRelacionados(camposRelacionados);


                tablaStacked->setCurrentWidget(tablaDesign);
                tablaDesign->actualizarPropiedades();
            } catch (const std::runtime_error& e) {
                QMessageBox::critical(this, "Error de Recarga",
                                      QString("No se pudo recargar el diseño de la tabla. Error: %1").arg(e.what()));
            }
        }
    }

    // Reconectar combo
    connect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        vistaHojaDatos = (index == 1);
        cambiarVista();
    });

    actualizarConexionesBotones();
    mostrarRibbonInicio();
    qDebug() << "✅ Vista cambiada para tabla:" << tablaActualNombre;
}


void MainWindow::abrirRelaciones()
{
    // 🔹 Verificar si ya existe una pestaña "Relaciones"
    for (int i = 0; i < zonaCentral->count(); ++i) {
        if (zonaCentral->tabText(i) == "Relaciones") {
            zonaCentral->setCurrentIndex(i); // seleccionar pestaña existente

            // ⭐ REFRESCAR LA PESTAÑA EXISTENTE
            RelacionesWidget *relacionesWidget = qobject_cast<RelacionesWidget*>(zonaCentral->widget(i));
            if (relacionesWidget) {
                relacionesWidget->refrescarTablas();
            }

            return; // no crear otra
        }
    }

    // 🔹 Guardar metadatos de la tabla actual si hay una abierta
    if (tablaActual) {
        VistaDiseno *tablaDesign = tablaActual->property("tablaDesign").value<VistaDiseno*>();
        VistaDatos *tablaDataSheet = tablaActual->property("tablaDataSheet").value<VistaDatos*>();
        if (tablaDataSheet) tablaDataSheet->cargarRelaciones("relationships.dat");

        Metadata meta(tablaActualNombre);
        if (tablaDesign) meta.campos = tablaDesign->obtenerCampos();
        if (tablaDataSheet) meta.registros = tablaDataSheet->obtenerRegistros(meta.campos);

        try {
            meta.guardar();
        } catch (const std::runtime_error &e) {
            QMessageBox::warning(this, "Error al guardar", e.what());
            return;
        }
    }

    // 🔹 Crear nueva pestaña de relaciones solo si no existía
    RelacionesWidget *relacionesWidget = new RelacionesWidget();
    int tabIndex = zonaCentral->addTab(relacionesWidget, "Relaciones");
    zonaCentral->setCurrentIndex(tabIndex);

    connect(relacionesWidget, &RelacionesWidget::cerrada,
            this, &MainWindow::cerrarRelacionesYVolver);

    connect(relacionesWidget, &RelacionesWidget::relacionCreada,
            this, &MainWindow::guardarRelacionEnBD);

    connect(this, &MainWindow::actualizarRelaciones, this, [this]() {
        for (int i = 0; i < zonaCentral->count(); ++i) {
            RelacionesWidget *relaciones = qobject_cast<RelacionesWidget*>(zonaCentral->widget(i));
            if (relaciones) {
                relaciones->refrescarTablas();
                relaciones->refrescarListaTablas();
                break;
            }
        }
    });

    mostrarRibbonInicio();
}

void MainWindow::guardarRelacionEnBD(const QString &tabla1, const QString &campo1,
                                     const QString &tabla2, const QString &campo2)
{
    qDebug() << "💾 Guardando relación:" << tabla1 << "." << campo1
             << "->" << tabla2 << "." << campo2;

    // Verificar que la relación no existe ya
    QFile relacionesFile("relationships.dat");
    if (relacionesFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&relacionesFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split("|");
            if (parts.size() == 4) {
                if (parts[0] == tabla1 && parts[1] == campo1 &&
                    parts[2] == tabla2 && parts[3] == campo2) {
                    qDebug() << "⚠️ Relación ya existe, no se guardará duplicada";
                    relacionesFile.close();
                    return;
                }
            }
        }
        relacionesFile.close();
    }

    // Guardar nueva relación
    if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream stream(&relacionesFile);
        stream << tabla1 << "|" << campo1 << "|" << tabla2 << "|" << campo2 << "\n";
        relacionesFile.close();

        qDebug() << "✅ Relación guardada correctamente";

        // ⭐ ACTUALIZAR TODAS LAS TABLAS ABIERTAS
        actualizarTablasAbiertasConRelaciones();

        // Emitir señal para actualizar otros componentes
        emit actualizarRelaciones();
    } else {
        qDebug() << "❌ Error al guardar relación en archivo";
    }
}
// ⭐ NUEVO MÉTODO: Eliminar relación de BD (si no existe, agregarlo)
void MainWindow::eliminarRelacionDeBD(const QString &tabla1, const QString &campo1,
                                      const QString &tabla2, const QString &campo2)
{
    qDebug() << "Eliminando relación:" << tabla1 << "." << campo1
             << "->" << tabla2 << "." << campo2;

    QFile relacionesFile("relationships.dat");
    if (!relacionesFile.open(QIODevice::ReadOnly)) {
        qDebug() << "No se pudo abrir archivo de relaciones";
        return;
    }

    QStringList relacionesRestantes;
    QTextStream in(&relacionesFile);
    bool relacionEliminada = false;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList parts = line.split("|");
        if (parts.size() == 4) {
            QString t1 = parts[0].trimmed();
            QString c1 = parts[1].trimmed();
            QString t2 = parts[2].trimmed();
            QString c2 = parts[3].trimmed();

            // COMPARACIÓN EXACTA - eliminar solo la relación específica
            if (t1 == tabla1 && c1 == campo1 && t2 == tabla2 && c2 == campo2) {
                relacionEliminada = true;
                qDebug() << "Relación encontrada y marcada para eliminación";
            } else {
                // Mantener todas las demás relaciones
                relacionesRestantes.append(line);
            }
        }
    }
    relacionesFile.close();

    if (!relacionEliminada) {
        qDebug() << "ADVERTENCIA: No se encontró la relación exacta a eliminar";
        qDebug() << "Buscaba:" << tabla1 << "|" << campo1 << "|" << tabla2 << "|" << campo2;
        return;
    }

    // REESCRIBIR completamente el archivo
    if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&relacionesFile);
        for (const QString &relacion : relacionesRestantes) {
            out << relacion << "\n";
        }
        relacionesFile.close();

        qDebug() << "Relación eliminada correctamente del archivo";
        qDebug() << "Relaciones restantes:" << relacionesRestantes.size();

        // Actualizar todas las tablas abiertas DESPUÉS de confirmar eliminación del archivo
        actualizarTablasAbiertasConRelaciones();

        // Emitir señal para actualizar otros componentes
        emit actualizarRelaciones();
    } else {
        qDebug() << "ERROR: No se pudo reescribir archivo de relaciones";
    }
}

void MainWindow::cerrarRelacionesYVolver()
{
    // Buscar la pestaña de relaciones
    for (int i = 0; i < zonaCentral->count(); ++i) {
        QWidget *tabWidget = zonaCentral->widget(i);
        if (qobject_cast<RelacionesWidget*>(tabWidget)) {
            zonaCentral->removeTab(i);
            break;
        }
    }

    qDebug() << "Ventana de relaciones cerrada";
}

void MainWindow::cerrarTab(int index)
{
    if (index == 0 && !zonaCentral->isTabEnabled(0)) {
        return;
    }

    QString tabName = zonaCentral->tabText(index);

    // 🔹 Evitar guardar consultas y relaciones como tablas
    if (tabName.startsWith("Diseño de Consulta") ||
        tabName.startsWith("Consulta") ||
        tabName.startsWith("Relaciones")) {
        zonaCentral->removeTab(index);

        if (zonaCentral->count() == 1 && !zonaCentral->isTabEnabled(0)) {
            zonaCentral->setCurrentIndex(0);
        }
        return;
    }

    QWidget *tablaContainer = zonaCentral->widget(index);
    if (tablaContainer) {
        VistaDiseno *tablaDesign = tablaContainer->property("tablaDesign").value<VistaDiseno*>();
        VistaDatos *tablaDataSheet = tablaContainer->property("tablaDataSheet").value<VistaDatos*>();

        Metadata meta(zonaCentral->tabText(index));

        if (tablaDesign) {
            meta.campos = tablaDesign->obtenerCampos();
        }
        if (tablaDataSheet) {
            meta.registros = tablaDataSheet->obtenerRegistros(meta.campos);
        }

        try {
            meta.guardar();   // guarda estructura + registros en disco
        } catch (const std::runtime_error &e) {
            QMessageBox::warning(this, "Error al guardar", e.what());
        }
    }

    zonaCentral->removeTab(index);

    if (zonaCentral->count() == 1 && !zonaCentral->isTabEnabled(0)) {
        zonaCentral->setCurrentIndex(0);
    }
}


bool MainWindow::nombreTablaEsUnico(const QString &nombreTabla) {
    QDir dir(QDir::currentPath() + "/tables");
    if (!dir.exists()) {
        return true;
    }

    QStringList archivos = dir.entryList(QStringList() << "*.meta", QDir::Files);
    foreach (const QString &archivo, archivos) {
        if (archivo == nombreTabla + ".meta") {
            return false;
        }
    }
    return true;
}

void MainWindow::crearNuevaTabla() {
    // Pide un nombre
    bool ok;
    QString nombreTabla = QInputDialog::getText(this, tr("Nueva Tabla"),
                                                tr("Nombre de la tabla:"),
                                                QLineEdit::Normal,
                                                "TablaNueva", &ok);
    if (!ok || nombreTabla.isEmpty()) return;

    // Validar nombre único
    if (!nombreTablaEsUnico(nombreTabla)) {
        QMessageBox::critical(this, tr("Tabla NO creada"), tr("Nombre ya registrado, ingrese uno diferente."));
        return;
    }

    // Crear metadata inicial con un campo ID
    Metadata meta(nombreTabla);
    Campo c;
    c.nombre = "ID";
    c.tipo = "NUMERO";
    c.propiedad = "entero";
    c.esPK = true;
    meta.campos.append(c);

    // Guardar archivo .meta
    try {
        meta.guardar();
    } catch (const std::runtime_error &e) {
        QMessageBox::critical(this, "Error", QString("No se pudo crear la tabla: %1").arg(e.what()));
        return;
    }

    // 🔹 Inicia la creación del QListWidgetItem con el widget personalizado
    QListWidgetItem *item = new QListWidgetItem(listaTablas);
    item->setData(Qt::UserRole, nombreTabla); // Almacenar el nombre de la tabla

    QWidget* itemWidget = new QWidget();
    QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
    QLabel* nameLabel = new QLabel(nombreTabla);
    QToolButton* menuButton = new QToolButton();
    menuButton->setIcon(QIcon(":/imgs/menu.png"));
    menuButton->setAutoRaise(true);

    QMenu* menu = new QMenu(this);
    QAction* renameAction = menu->addAction(tr("Editar Nombre"));
    QAction* deleteAction = menu->addAction(tr("Eliminar"));

    // Conectar las acciones a las funciones correspondientes
    connect(renameAction, &QAction::triggered, this, [this, nombreTabla]() {
        editarNombreTabla(nombreTabla);
    });
    connect(deleteAction, &QAction::triggered, this, [this, nombreTabla]() {
        eliminarTabla(nombreTabla);
    });
    menuButton->setMenu(menu);
    menuButton->setPopupMode(QToolButton::InstantPopup);

    itemLayout->addWidget(nameLabel);
    itemLayout->addStretch();
    itemLayout->addWidget(menuButton);
    itemLayout->setContentsMargins(0, 0, 0, 0);

    // Es importante ajustar el tamaño y añadir el item a la lista
    item->setSizeHint(itemWidget->sizeHint());
    listaTablas->addItem(item);
    listaTablas->setItemWidget(item, itemWidget);

    // Crear un widget contenedor para las vistas de la tabla
    QWidget *tablaContainer = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(tablaContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    // Crear stacked widget para alternar entre vista diseño y hoja de datos
    QStackedWidget *tablaStacked = new QStackedWidget();

    // Crear ambas vistas
    VistaDiseno *tablaDesign = new VistaDiseno();
    VistaDatos *tablaDataSheet = new VistaDatos();

    // ⭐ CRÍTICO: Establecer nombre de tabla ANTES de cargar cualquier cosa
    tablaDataSheet->establecerNombreTabla(nombreTabla);
    // Cargar campos en vista diseño
    tablaDesign->cargarCampos(meta.campos);
    tablaDesign->setNombreTabla(nombreTabla);

    // Cargar metadata en vista datos
    tablaDataSheet->cargarDesdeMetadata(meta);

    // Añadir ambas vistas al stacked widget
    tablaStacked->addWidget(tablaDesign);
    tablaStacked->addWidget(tablaDataSheet);

    // Configurar la vista inicial según el modo actual (vista diseño por defecto)
    tablaStacked->setCurrentWidget(tablaDesign);

    containerLayout->addWidget(tablaStacked);

    // Añadir el nuevo tab
    int newTabIndex = zonaCentral->addTab(tablaContainer, nombreTabla);
    zonaCentral->setCurrentIndex(newTabIndex);

    // Almacenar referencias a las vistas en propiedades del widget contenedor
    tablaContainer->setProperty("tablaDesign", QVariant::fromValue(tablaDesign));
    tablaContainer->setProperty("tablaDataSheet", QVariant::fromValue(tablaDataSheet));
    tablaContainer->setProperty("tablaStacked", QVariant::fromValue(tablaStacked));

    // Actualizar tablaActual y tablaActualNombre
    tablaActual = tablaContainer;
    tablaActualNombre = nombreTabla;

    // Conectar señales según la vista actual (vista diseño)
    actualizarConexionesBotones();
    connect(btnLlavePrimaria, &QToolButton::clicked,
            tablaDesign, &VistaDiseno::establecerPK);

    // Conectar señal de modificación
    connect(tablaDesign, &VistaDiseno::metadatosModificados,
            this, &MainWindow::onMetadatosModificados);

    // Mostrar ribbon de Inicio
    mostrarRibbonInicio();

    qDebug() << "✅ Tabla creada y configurada:" << nombreTabla;


    // Abrir la nueva tabla
    abrirTabla(item);
}

void MainWindow::guardarTablasAbiertas()
{
    for (int i = 0; i < zonaCentral->count(); ++i) {

        QString tabName = zonaCentral->tabText(i);

        // 🔹 Ignorar consultas y relaciones
        if (tabName.startsWith("Diseño de Consulta") ||
            tabName.startsWith("Consulta") ||
            tabName.startsWith("Relaciones")) {
            continue;
        }

        QWidget *tabWidget = zonaCentral->widget(i);
        if (tabWidget && tabWidget != zonaCentral->widget(0)) {
            VistaDiseno *tablaDesign = tabWidget->property("tablaDesign").value<VistaDiseno*>();
            if (tablaDesign) {
                Metadata meta(tabName);
                meta.campos = tablaDesign->obtenerCampos();
                meta.guardar();
            }
        }
    }
}


void MainWindow::ordenarRegistros(Qt::SortOrder order)
{
    if (!tablaActual || !vistaHojaDatos) {
        return; // No hay tabla abierta o no estamos en la vista de hoja de datos
    }

    QStackedWidget *tablaStacked = tablaActual->property("tablaStacked").value<QStackedWidget*>();
    if (!tablaStacked) {
        return;
    }

    VistaDatos *tablaDataSheet = tablaStacked->findChild<VistaDatos*>();
    if (tablaDataSheet) {
        // Llamar a la función de ordenamiento en VistaDatos
        tablaDataSheet->ordenar(order);
    }
}

QSet<QString> MainWindow::obtenerCamposRelacionados(const QString& nombreTabla) {
    QSet<QString> campos;

    QFile relacionesFile("relationships.dat");
    if (relacionesFile.open(QIODevice::ReadOnly)) {
        QTextStream in(&relacionesFile);
        while (!in.atEnd()) {
            QString line = in.readLine();
            QStringList parts = line.split("|");
            if (parts.size() == 4) {
                if (parts[0] == nombreTabla) {
                    campos.insert(parts[1]); // Campo origen
                }
                if (parts[2] == nombreTabla) {
                    campos.insert(parts[3]); // Campo destino
                }
            }
        }
        relacionesFile.close();
    }

    return campos;
}

void MainWindow::onMetadatosModificados() {
    // Actualizar la lista de campos relacionados
    if (!tablaActualNombre.isEmpty()) {
        QSet<QString> camposRelacionados = obtenerCamposRelacionados(tablaActualNombre);

        // Obtener la vista de diseño actual
        QWidget *currentTab = zonaCentral->currentWidget();
        if (currentTab) {
            VistaDiseno *tablaDesign = currentTab->property("tablaDesign").value<VistaDiseno*>();
            if (tablaDesign) {
                tablaDesign->setCamposRelacionados(camposRelacionados);
                QTimer::singleShot(100, tablaDesign, &VistaDiseno::actualizarEstadoCampos);
            }
        }
    }
    actualizarTablasAbiertasConRelaciones();
    emit actualizarRelaciones();
}

void MainWindow::crearNuevaConsulta() {
    ConsultaWidget *cw = new ConsultaWidget();
    int idx = zonaCentral->addTab(cw, "Diseño de Consulta");
    zonaCentral->setCurrentIndex(idx);
}


void MainWindow::eliminarTabla(const QString& nombreTabla)
{
    // Verificar si la tabla tiene relaciones
    if (tablaTieneRelaciones(nombreTabla)) {
        return;
    }

    // Confirmar con el usuario antes de eliminar
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Eliminar Tabla",
                                  "¿Está seguro de que desea eliminar la tabla '" + nombreTabla + "'? Esta acción es irreversible.",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::No) {
        return;
    }
// 1. Cerrar la pestaña si está abierta
for (int i = 0; i < zonaCentral->count(); ++i) {
    if (zonaCentral->tabText(i) == nombreTabla) {
        zonaCentral->removeTab(i);
        break;
    }
}

// 2. Eliminar de la lista de tablas (QListWidget)
QListWidgetItem* itemToRemove = nullptr;
for (int i = 0; i < listaTablas->count(); ++i) {
    QListWidgetItem* item = listaTablas->item(i);
    if (item->data(Qt::UserRole).toString() == nombreTabla) {
        itemToRemove = item;
        break;
    }
}

if (itemToRemove) {
    listaTablas->takeItem(listaTablas->row(itemToRemove));
    delete itemToRemove;
}

// 3. Eliminar los archivos de metadatos y datos
QString metaPath = QDir::currentPath() + "/tables/" + nombreTabla + ".meta";
QString dataPath = QDir::currentPath() + "/tables/" + nombreTabla + ".dat";

QFile metaFile(metaPath);
QFile dataFile(dataPath);

bool metaEliminado = metaFile.remove();
bool dataEliminado = dataFile.exists() ? dataFile.remove() : true;

if (metaEliminado && dataEliminado) {
    QMessageBox::information(this, "Eliminar Tabla", "La tabla '" + nombreTabla + "' ha sido eliminada correctamente.");
} else {
    QMessageBox::warning(this, "Eliminar Tabla",
                         "No se pudieron eliminar todos los archivos de la tabla.\n"
                         "Meta: " + QString(metaEliminado ? "Sí" : "No") + "\n" +
                             "Data: " + QString(dataEliminado ? "Sí" : "No"));
}

// 4. Actualizar la lista en RelacionesWidget si está abierta
emit actualizarRelaciones();
}


void MainWindow::editarNombreTabla(const QString& nombreTabla)
{
    bool ok;
    QString nuevoNombre = QInputDialog::getText(this, "Editar Nombre",
                                                "Nuevo nombre para la tabla '" + nombreTabla + "':",
                                                QLineEdit::Normal,
                                                nombreTabla, &ok);

    // 1. Validar la entrada del usuario
    if (!ok || nuevoNombre.isEmpty() || nuevoNombre == nombreTabla) {
        return;
    }

    // 2. Verificar que el nuevo nombre no exista
    if (!nombreTablaEsUnico(nuevoNombre)) {
        QMessageBox::critical(this, "Error al Renombrar", "El nombre '" + nuevoNombre + "' ya existe.");
        return;
    }

    // Rutas de archivos
    QString viejoMetaPath = QDir::currentPath() + "/tables/" + nombreTabla + ".meta";
    QString viejoDataPath = QDir::currentPath() + "/tables/" + nombreTabla + ".dat";
    QString nuevoMetaPath = QDir::currentPath() + "/tables/" + nuevoNombre + ".meta";
    QString nuevoDataPath = QDir::currentPath() + "/tables/" + nuevoNombre + ".dat";

    // 3. Renombrar el archivo .meta
    // Si el archivo .meta no existe, no se puede continuar.
    if (!QFile::exists(viejoMetaPath)) {
        QMessageBox::critical(this, "Error", "El archivo de metadatos de la tabla no se encontró.");
        return;
    }
    if (!QFile::rename(viejoMetaPath, nuevoMetaPath)) {
        QMessageBox::critical(this, "Error", "No se pudo renombrar el archivo .meta. Verifique los permisos.");
        return;
    }

    // 4. Renombrar el archivo .dat (opcional, no es crítico si falla)
    // Se verifica si el archivo existe antes de intentar renombrarlo.
    if (QFile::exists(viejoDataPath)) {
        if (!QFile::rename(viejoDataPath, nuevoDataPath)) {
            QMessageBox::warning(this, "Advertencia", "No se pudo renombrar el archivo .dat. La tabla ha sido renombrada, pero podría haber una inconsistencia.");
        }
    }

    // 5. Actualizar el nombre dentro del archivo .meta
    // Se carga la metadata, se cambia el nombre y se guarda de nuevo.
    try {
        Metadata meta = Metadata::cargar(nuevoMetaPath);
        meta.nombreTabla = nuevoNombre;
        meta.guardar();
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", "No se pudo actualizar el nombre dentro del archivo .meta: " + QString(e.what()));
        // Se revierte el cambio de nombre del archivo para evitar inconsistencias
        QFile::rename(nuevoMetaPath, viejoMetaPath);
        return;
    }

    // 6. Renombrar relaciones en relationships.dat (si es necesario)
    renombrarTablaEnRelaciones(nombreTabla, nuevoNombre);

    // 7. Actualizar la interfaz de usuario
    // Buscar y actualizar el item en la lista de tablas
    QListWidgetItem* itemToRename = nullptr;
    for (int i = 0; i < listaTablas->count(); ++i) {
        QListWidgetItem* item = listaTablas->item(i);
        if (item->data(Qt::UserRole).toString() == nombreTabla) {
            itemToRename = item;
            break;
        }
    }
    if (itemToRename) {
        itemToRename->setData(Qt::UserRole, nuevoNombre);
        if (listaTablas->itemWidget(itemToRename)) {
            QLabel* label = listaTablas->itemWidget(itemToRename)->findChild<QLabel*>();
            if (label) {
                label->setText(nuevoNombre);
            }
        }
    } else {
        cargarListaTablasDesdeArchivos(); // Recargar la lista si el item no se encuentra
    }

    // 8. Actualizar la pestaña si la tabla está abierta
    for (int i = 0; i < zonaCentral->count(); ++i) {
        if (zonaCentral->tabText(i) == nombreTabla) {
            zonaCentral->setTabText(i, nuevoNombre);

            QWidget* tabWidget = zonaCentral->widget(i);
            if (tabWidget) {
                VistaDiseno* tablaDesign = tabWidget->property("tablaDesign").value<VistaDiseno*>();
                VistaDatos* tablaDataSheet = tabWidget->property("tablaDataSheet").value<VistaDatos*>();

                if (tablaDesign) {
                    tablaDesign->setNombreTabla(nuevoNombre);
                    // No es necesario recargar campos si se actualiza el nombre en memoria
                }
                if (tablaDataSheet) {
                    tablaDataSheet->establecerNombreTabla(nuevoNombre);
                    // No es necesario recargar datos si se actualiza el nombre en memoria
                }
            }
            break;
        }
    }

    // 9. Emitir señales para actualizar otras partes de la aplicación
    actualizarTablasAbiertasConRelaciones();
    emit actualizarRelaciones();

    QMessageBox::information(this, "Renombrar Tabla", "La tabla ha sido renombrada a '" + nuevoNombre + "'.");
}

void MainWindow::eliminarRelacionesDeTabla(const QString& nombreTabla)
{
    qDebug() << "🗑️ Eliminando todas las relaciones de la tabla:" << nombreTabla;

    QFile relacionesFile("relationships.dat");
    if (!relacionesFile.open(QIODevice::ReadOnly)) {
        qDebug() << "❌ No se pudo abrir archivo de relaciones para eliminar";
        return;
    }

    QStringList relaciones;
    QTextStream in(&relacionesFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split("|");
        if (parts.size() == 4) {
            // No agregar relaciones que involucren a la tabla eliminada
            if (parts[0] == nombreTabla || parts[2] == nombreTabla) {
                qDebug() << "❌ Eliminando relación:" << line;
                continue;
            }
            relaciones.append(line);
        }
    }
    relacionesFile.close();

    // Reescribir el archivo sin las relaciones eliminadas
    if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&relacionesFile);
        for (const QString &relacion : relaciones) {
            out << relacion << "\n";
        }
        relacionesFile.close();
        qDebug() << "✅ Relaciones de la tabla" << nombreTabla << "eliminadas correctamente";
    } else {
        qDebug() << "❌ Error al reescribir archivo de relaciones";
    }
}

void MainWindow::renombrarTablaEnRelaciones(const QString& nombreViejo, const QString& nombreNuevo)
{
    qDebug() << "✏️ Renombrando tabla en relaciones:" << nombreViejo << "->" << nombreNuevo;

    QFile relacionesFile("relationships.dat");
    if (!relacionesFile.open(QIODevice::ReadOnly)) {
        qDebug() << "❌ No se pudo abrir archivo de relaciones para renombrar";
        return;
    }

    QStringList relaciones;
    QTextStream in(&relacionesFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split("|");
        if (parts.size() == 4) {
            // Renombrar la tabla donde aparezca
            if (parts[0] == nombreViejo) {
                parts[0] = nombreNuevo;
            }
            if (parts[2] == nombreViejo) {
                parts[2] = nombreNuevo;
            }
            line = parts.join("|");
            relaciones.append(line);
        }
    }
    relacionesFile.close();

    // Reescribir el archivo con los nombres actualizados
    if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream out(&relacionesFile);
        for (const QString &relacion : relaciones) {
            out << relacion << "\n";
        }
        relacionesFile.close();
        qDebug() << "✅ Tabla renombrada en relaciones correctamente";
    } else {
        qDebug() << "❌ Error al reescribir archivo de relaciones";
    }
}



void MainWindow::actualizarNombreEnArchivoMeta(const QString& nombreTabla, const QString& nombreNuevo)
{
    QString filePath = QDir::currentPath() + "/tables/" + nombreTabla + ".meta";
    QFile metaFile(filePath);

    if (!metaFile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        qDebug() << "❌ No se pudo abrir el archivo .meta para actualizar el nombre:" << filePath;
        return;
    }

    // Leer todo el contenido
    QTextStream in(&metaFile);
    QStringList lineas;
    while (!in.atEnd()) {
        lineas << in.readLine();
    }

    // Actualizar la primera línea (que contiene "Tabla: [nombre]")
    if (!lineas.isEmpty() && lineas[0].startsWith("Tabla: ")) {
        lineas[0] = "Tabla: " + nombreTabla;
    }

    // Volver al inicio y escribir el contenido actualizado
    metaFile.resize(0);
    QTextStream out(&metaFile);
    for (const QString& linea : lineas) {
        out << linea << "\n";
    }

    metaFile.close();
    qDebug() << "✅ Nombre actualizado en archivo .meta:" << nombreTabla;
}

void MainWindow::cargarListaTablasDesdeArchivos()
{
    listaTablas->clear();

    QDir dir(QDir::currentPath() + "/tables");
    if (!dir.exists()) {
        dir.mkpath(".");
        return;
    }

    QStringList archivosMeta = dir.entryList(QStringList() << "*.meta", QDir::Files);

    for (const QString &fileName : archivosMeta) {
        try {
            QString filePath = dir.filePath(fileName);
            Metadata meta = Metadata::cargar(filePath);
            if (!meta.nombreTabla.isEmpty()) {
                QListWidgetItem *item = new QListWidgetItem(listaTablas);
                item->setData(Qt::UserRole, meta.nombreTabla);

                // Crear custom widget para el item
                QWidget* itemWidget = new QWidget();
                QHBoxLayout* itemLayout = new QHBoxLayout(itemWidget);
                QLabel* nameLabel = new QLabel(meta.nombreTabla);
                QToolButton* menuButton = new QToolButton();
                menuButton->setIcon(QIcon(":/imgs/menu.png"));
                menuButton->setAutoRaise(true);

                QMenu* menu = new QMenu(this);
                QAction* renameAction = menu->addAction("Editar Nombre");
                QAction* deleteAction = menu->addAction("Eliminar");

                connect(renameAction, &QAction::triggered, this, [this, meta]() {
                    editarNombreTabla(meta.nombreTabla);
                });
                connect(deleteAction, &QAction::triggered, this, [this, meta]() {
                    eliminarTabla(meta.nombreTabla);
                });
                menuButton->setMenu(menu);
                menuButton->setPopupMode(QToolButton::InstantPopup);

                itemLayout->addWidget(nameLabel);
                itemLayout->addStretch();
                itemLayout->addWidget(menuButton);
                itemLayout->setContentsMargins(0, 0, 0, 0);

                item->setSizeHint(itemWidget->sizeHint());
                listaTablas->addItem(item);
                listaTablas->setItemWidget(item, itemWidget);
            }
        } catch (const std::runtime_error& e) {
            qDebug() << "Error al cargar tabla desde" << fileName << ":" << e.what();
        }
    }
}

bool MainWindow::tablaTieneRelaciones(const QString& nombreTabla)
{
    QFile relacionesFile("relationships.dat");
    if (!relacionesFile.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream in(&relacionesFile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split("|");
        if (parts.size() == 4) {
            if (parts[0] == nombreTabla || parts[2] == nombreTabla) {
                relacionesFile.close();
                // El error está aquí. Si hay relaciones, el método retorna
                // inmediatamente, impidiendo la eliminación de los archivos.
                return true;
            }
        }
    }

    relacionesFile.close();
    return false;
}


