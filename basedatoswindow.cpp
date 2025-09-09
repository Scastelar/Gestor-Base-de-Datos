#include "basedatoswindow.h"
#include "datasheetwidget.h"
#include "relacioneswidget.h"
#include "metadata.h"
#include "tablacentralwidget.h"
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

BaseDatosWindow::BaseDatosWindow(QWidget *parent)
    : QMainWindow(parent), vistaHojaDatos(false), filtroActivo(false), tablaActual(nullptr),
    listaTablas(nullptr), zonaCentral(nullptr), ribbonInicio(nullptr), ribbonCrear(nullptr),
    btnLlavePrimaria(nullptr), btnFiltrar(nullptr), btnAscendente(nullptr), btnDescendente(nullptr),
    btnInsertarFila(nullptr), btnEliminarFila(nullptr), btnRelaciones(nullptr), comboVista(nullptr),
    filasLayout(nullptr), botonesFilasWidget(nullptr), botonesFilasVLayout(nullptr), botonesFilasHLayout(nullptr)
{
    QIcon iconTabla(":/imgs/table.png");
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
        Metadata meta = Metadata::cargar(dir.filePath(fileName));
        if (!meta.nombreTabla.isEmpty()) {
            listaTablas->addItem(new QListWidgetItem(iconTabla, meta.nombreTabla));
        }
    }

    connect(listaTablas, &QListWidget::itemClicked, this, &BaseDatosWindow::abrirTabla);

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
    connect(zonaCentral, &QTabWidget::tabCloseRequested, this, &BaseDatosWindow::cerrarTab);
    connect(zonaCentral, &QTabWidget::currentChanged, this, &BaseDatosWindow::cambiarTablaActual);

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
   // connect(btnInsertarFila, &QToolButton::clicked, this, &BaseDatosWindow::insertarFilaActual);
   // connect(btnEliminarFila, &QToolButton::clicked, this, &BaseDatosWindow::eliminarFilaActual);
}

void BaseDatosWindow::aplicarEstilos()
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

void BaseDatosWindow::crearMenus()
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

void BaseDatosWindow::crearRibbonTabs()
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
    connect(btnInicio, &QToolButton::clicked, this, &BaseDatosWindow::mostrarRibbonInicio);

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
    connect(btnCrear, &QToolButton::clicked, this, &BaseDatosWindow::mostrarRibbonCrear);

    ribbonTabLayout->addWidget(btnInicio);
    ribbonTabLayout->addWidget(btnCrear);
    ribbonTabLayout->addStretch();

    menuBar()->setCornerWidget(ribbonTabWidget, Qt::TopLeftCorner);
}

void BaseDatosWindow::crearToolbars()
{
    crearRibbonInicio();
    crearRibbonCrear();
}

void BaseDatosWindow::crearRibbonInicio()
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
    QVBoxLayout *filasMainLayout = new QVBoxLayout(filasFrame);

    // Crear los botones
    btnInsertarFila = crearBotonRibbon(":/imgs/insert-row.png", "Insertar Fila");
    btnEliminarFila = crearBotonRibbon(":/imgs/delete-row.png", "Eliminar Fila");

    // Crear widget contenedor y layouts
    botonesFilasWidget = new QWidget();
    botonesFilasVLayout = new QVBoxLayout(botonesFilasWidget);
    botonesFilasHLayout = new QHBoxLayout();

    // Configurar layouts
    botonesFilasVLayout->setSpacing(5);
    botonesFilasVLayout->setContentsMargins(0, 0, 0, 0);
    botonesFilasHLayout->setSpacing(5);
    botonesFilasHLayout->setContentsMargins(0, 0, 0, 0);

    // Agregar botones a ambos layouts
    botonesFilasVLayout->addWidget(btnInsertarFila);
    botonesFilasVLayout->addWidget(btnEliminarFila);
    botonesFilasHLayout->addWidget(btnInsertarFila);
    botonesFilasHLayout->addWidget(btnEliminarFila);

    // Usar layout vertical por defecto
    botonesFilasWidget->setLayout(botonesFilasVLayout);
    filasMainLayout->addWidget(botonesFilasWidget);

    // Sección Relaciones
    QFrame *relacionesFrame = crearSeccionRibbon("Relaciones");
    QVBoxLayout *relacionesLayout = new QVBoxLayout(relacionesFrame);

    btnRelaciones = crearBotonRibbon(":/imgs/relationships.png", "Relaciones");
    connect(btnRelaciones, &QToolButton::clicked, this, &BaseDatosWindow::abrirRelaciones);
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

void BaseDatosWindow::crearRibbonCrear()
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

    QToolButton *btnTabla = crearBotonRibbon(":/imgs/table.png", "Tabla");
    QToolButton *btnDisenoTabla = crearBotonRibbon(":/imgs/form-design.png", "Diseño");
    connect(btnTabla, &QToolButton::clicked, this, &BaseDatosWindow::crearNuevaTabla);

    tablasLayout->addWidget(btnTabla);
    tablasLayout->addWidget(btnDisenoTabla);

    // Sección Consultas
    QFrame *queriesFrame = crearSeccionRibbon("Consultas");
    QHBoxLayout *queriesLayout = new QHBoxLayout(queriesFrame);

    QToolButton *btnConsulta = crearBotonRibbon(":/imgs/query.png", "Consulta");
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
}

QFrame* BaseDatosWindow::crearSeccionRibbon(const QString &titulo)
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

QToolButton* BaseDatosWindow::crearBotonRibbon(const QString &iconPath, const QString &texto)
{
    QToolButton *btn = new QToolButton();
    btn->setIcon(QIcon(iconPath));
    btn->setText(texto);
    btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btn->setStyleSheet(".ribbonButton");
    return btn;
}

void BaseDatosWindow::mostrarRibbonInicio()
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

    // Cambiar layout de botones de filas según la vista
    if (botonesFilasWidget) {
        // Limpiar el layout actual
        QLayout *layoutActual = botonesFilasWidget->layout();
        if (layoutActual) {
            QLayoutItem *item;
            while ((item = layoutActual->takeAt(0)) != nullptr) {
                delete item;
            }
            delete layoutActual;
        }

        // Crear nuevo layout según la vista
        if (esHojaDatos) {
            // En Vista Hoja de Datos: layout horizontal
            QHBoxLayout *nuevoLayout = new QHBoxLayout(botonesFilasWidget);
            nuevoLayout->addWidget(btnInsertarFila);
            nuevoLayout->addWidget(btnEliminarFila);
            nuevoLayout->setSpacing(5);
            nuevoLayout->setContentsMargins(0, 0, 0, 0);
        } else {
            // En Vista Diseño: layout vertical
            QVBoxLayout *nuevoLayout = new QVBoxLayout(botonesFilasWidget);
            nuevoLayout->addWidget(btnInsertarFila);
            nuevoLayout->addWidget(btnEliminarFila);
            nuevoLayout->setSpacing(5);
            nuevoLayout->setContentsMargins(0, 0, 0, 0);

            // Actualizar propiedades para la vista diseño si existe una tabla abierta
            QWidget *currentTab = zonaCentral->currentWidget();
            if (currentTab && currentTab != zonaCentral->widget(0)) {
                TablaCentralWidget *tablaDesign = currentTab->property("tablaDesign").value<TablaCentralWidget*>();
                if (tablaDesign) {
                    tablaDesign->actualizarPropiedades();
                }
            }
        }
    }

    // Actualizar el estado de los botones en el corner widget
    QWidget *cornerWidget = menuBar()->cornerWidget(Qt::TopLeftCorner);
    if (cornerWidget) {
        QList<QToolButton*> buttons = cornerWidget->findChildren<QToolButton*>();
        if (buttons.size() >= 2) {
            buttons[0]->setChecked(true);
            buttons[1]->setChecked(false);
        }
    }
}

void BaseDatosWindow::mostrarRibbonCrear()
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

int BaseDatosWindow::encontrarTablaEnTabs(const QString &nombreTabla) const
{
    for (int i = 0; i < zonaCentral->count(); ++i) {
        if (zonaCentral->tabText(i) == nombreTabla) {
            return i;
        }
    }
    return -1;
}

void BaseDatosWindow::abrirTabla(QListWidgetItem *item)
{
    if (!item) return;

    tablaActualNombre = item->text();

    // Cargar metadata
    Metadata meta = Metadata::cargar(QDir::currentPath() + "/tables/" + tablaActualNombre + ".meta");

    // Verificar si la tabla ya está abierta
    int tabIndex = encontrarTablaEnTabs(tablaActualNombre);
    if (tabIndex != -1) {
        zonaCentral->setCurrentIndex(tabIndex);
        return;
    }

    // Crear un widget contenedor para las vistas de la tabla
    QWidget *tablaContainer = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(tablaContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    // Crear stacked widget para alternar entre vista diseño y hoja de datos
    QStackedWidget *tablaStacked = new QStackedWidget();

    // Crear ambas vistas
    TablaCentralWidget *tablaDesign = new TablaCentralWidget();
    DataSheetWidget *tablaDataSheet = new DataSheetWidget();

    // Cargar estructura en ambas vistas
    tablaDesign->cargarCampos(meta.campos);
    tablaDataSheet->cargarDesdeMetadata(meta);

    // Añadir ambas vistas al stacked widget
    tablaStacked->addWidget(tablaDesign);
    tablaStacked->addWidget(tablaDataSheet);

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
                tablaDataSheet, &DataSheetWidget::establecerPK);
    } else {
        connect(btnLlavePrimaria, &QToolButton::clicked,
                tablaDesign, &TablaCentralWidget::establecerPK);
    }

    // Mostrar ribbon de Inicio
    mostrarRibbonInicio();
}

void BaseDatosWindow::cambiarTablaActual(int index)
{
    // Si es la pestaña de inicio o no hay pestañas, limpiar referencia
    if (index == 0 || zonaCentral->count() <= 1) {
        tablaActual = nullptr;
        tablaActualNombre.clear();
        return;
    }

    // Obtener la tabla actual
    QWidget *currentTab = zonaCentral->widget(index);
    tablaActual = currentTab;
    tablaActualNombre = zonaCentral->tabText(index);

    // Actualizar conexiones de botones para la tabla actual
    actualizarConexionesBotones();
}

void BaseDatosWindow::actualizarConexionesBotones()
{
    if (!tablaActual) return;

    // Desconectar todas las conexiones previas
    disconnect(btnLlavePrimaria, &QToolButton::clicked, 0, 0);
    disconnect(btnInsertarFila, &QToolButton::clicked, 0, 0);
    disconnect(btnEliminarFila, &QToolButton::clicked, 0, 0);

    // Obtener las vistas de la tabla actual
    TablaCentralWidget *tablaDesign = tablaActual->property("tablaDesign").value<TablaCentralWidget*>();
    DataSheetWidget *tablaDataSheet = tablaActual->property("tablaDataSheet").value<DataSheetWidget*>();

    if (vistaHojaDatos && tablaDataSheet) {
        connect(btnLlavePrimaria, &QToolButton::clicked, tablaDataSheet, &DataSheetWidget::establecerPK);
        connect(btnInsertarFila, &QToolButton::clicked, tablaDataSheet, &DataSheetWidget::agregarRegistro);
        //connect(btnEliminarFila, &QToolButton::clicked, tablaDataSheet, &DataSheetWidget::eliminarRegistro);
    } else if (tablaDesign) {
        connect(btnLlavePrimaria, &QToolButton::clicked, tablaDesign, &TablaCentralWidget::establecerPK);
        connect(btnInsertarFila, &QToolButton::clicked, tablaDesign, &TablaCentralWidget::agregarCampo);
        connect(btnEliminarFila, &QToolButton::clicked, tablaDesign, &TablaCentralWidget::eliminarCampo);
    }


}

void BaseDatosWindow::insertarFilaActual()
{
    if (!tablaActual) return;

    if (vistaHojaDatos) {
        DataSheetWidget *tablaDataSheet = tablaActual->property("tablaDataSheet").value<DataSheetWidget*>();
        if (tablaDataSheet) tablaDataSheet->agregarRegistro();
    } else {
        TablaCentralWidget *tablaDesign = tablaActual->property("tablaDesign").value<TablaCentralWidget*>();
        if (tablaDesign) tablaDesign->agregarCampo();
    }
}

void BaseDatosWindow::eliminarFilaActual()
{
    if (!tablaActual) return;

    if (vistaHojaDatos) {
        DataSheetWidget *tablaDataSheet = tablaActual->property("tablaDataSheet").value<DataSheetWidget*>();
       // if (tablaDataSheet) tablaDataSheet->eliminarRegistro();
    } else {
        TablaCentralWidget *tablaDesign = tablaActual->property("tablaDesign").value<TablaCentralWidget*>();
        if (tablaDesign) tablaDesign->eliminarCampo();
    }
}


void BaseDatosWindow::cambiarVista()
{
    if (!tablaActual) return;

    QStackedWidget *tablaStacked = tablaActual->property("tablaStacked").value<QStackedWidget*>();
    TablaCentralWidget *tablaDesign = tablaActual->property("tablaDesign").value<TablaCentralWidget*>();
    DataSheetWidget *tablaDataSheet = tablaActual->property("tablaDataSheet").value<DataSheetWidget*>();

    if (!tablaStacked) return;

    // 🔹 Crear metadata con lo que esté actualmente en memoria
    Metadata meta(tablaActualNombre);

    if (tablaDesign) {
        meta.campos = tablaDesign->obtenerCampos();
    }
    if (tablaDataSheet) {
        meta.registros = tablaDataSheet->obtenerRegistros(meta.campos);
    }

    try {
        meta.guardar();  // guarda estructura + datos en .meta y .data
    } catch (const std::runtime_error &e) {
        QMessageBox::warning(this, "Error al guardar", e.what());
        return;
    }

    // 🔹 Desconectar combo temporalmente para evitar loops
    disconnect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), 0, 0);

    if (vistaHojaDatos) {
        comboVista->setCurrentIndex(1);

        if (tablaDataSheet) {
            // Recargar desde disco (ya trae campos + registros)
            Metadata recargada = Metadata::cargar(QDir::currentPath() + "/tables/" + tablaActualNombre + ".meta");
            tablaDataSheet->cargarDesdeMetadata(recargada);
            tablaStacked->setCurrentWidget(tablaDataSheet);
        }
    } else {
        comboVista->setCurrentIndex(0);

        if (tablaDesign) {
            Metadata recargada = Metadata::cargar(QDir::currentPath() + "/tables/" + tablaActualNombre + ".meta");
            tablaDesign->cargarCampos(recargada.campos);
            tablaStacked->setCurrentWidget(tablaDesign);
            tablaDesign->actualizarPropiedades();
        }
    }

    // 🔹 Reconectar combo
    connect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        vistaHojaDatos = (index == 1);
        cambiarVista();
    });

    actualizarConexionesBotones();
    mostrarRibbonInicio();
}

void BaseDatosWindow::abrirRelaciones()
{
    // Crear el widget de relaciones
    RelacionesWidget *relacionesWidget = new RelacionesWidget();

    // Añadir como nueva pestaña
    int tabIndex = zonaCentral->addTab(relacionesWidget, "Relaciones");
    zonaCentral->setCurrentIndex(tabIndex);

    // Conectar la señal de cierre
    connect(relacionesWidget, &RelacionesWidget::cerrada,
            this, &BaseDatosWindow::cerrarRelacionesYVolver);

    // Mostrar ribbon de Inicio (para mantener la UI consistente)
    mostrarRibbonInicio();

    qDebug() << "Ventana de relaciones abierta en nueva pestaña";
}

void BaseDatosWindow::cerrarRelacionesYVolver()
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

void BaseDatosWindow::cerrarTab(int index)
{
    if (index == 0 && !zonaCentral->isTabEnabled(0)) {
        return;
    }

    QWidget *tablaContainer = zonaCentral->widget(index);
    if (tablaContainer) {
        TablaCentralWidget *tablaDesign = tablaContainer->property("tablaDesign").value<TablaCentralWidget*>();
        DataSheetWidget *tablaDataSheet = tablaContainer->property("tablaDataSheet").value<DataSheetWidget*>();

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

bool BaseDatosWindow::nombreTablaEsUnico(const QString &nombreTabla) {
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

void BaseDatosWindow::crearNuevaTabla() {
    // Pide un nombre
    bool ok;
    QString nombreTabla = QInputDialog::getText(this, "Nueva Tabla",
                                                "Nombre de la tabla:",
                                                QLineEdit::Normal,
                                                "TablaNueva", &ok);
    if (!ok || nombreTabla.isEmpty()) return;

    // Validar nombre único
    if (!nombreTablaEsUnico(nombreTabla)) {
        QMessageBox::critical(this, "Tabla NO creada", "Nombre ya registrado, ingrese uno diferente.");
        return;
    }

    // Crear metadata inicial con un campo ID
    Metadata meta(nombreTabla);
    Campo c;
    c.nombre = "ID";
    c.tipo = "INT";
    meta.campos.append(c);

    // Guardar archivo .meta
    meta.guardar();

    // Agregar tabla a la lista lateral
    QIcon iconTabla(":/imgs/table.png");
    listaTablas->addItem(new QListWidgetItem(iconTabla, nombreTabla));

    // Crear un widget contenedor para las vistas de la tabla
    QWidget *tablaContainer = new QWidget();
    QVBoxLayout *containerLayout = new QVBoxLayout(tablaContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);

    // Crear stacked widget para alternar entre vista diseño y hoja de datos
    QStackedWidget *tablaStacked = new QStackedWidget();

    // Crear ambas vistas
    TablaCentralWidget *tablaDesign = new TablaCentralWidget();
    DataSheetWidget *tablaDataSheet = new DataSheetWidget();

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

    // Conectar señales según la vista actual (vista diseño)
    actualizarConexionesBotones();
    connect(btnLlavePrimaria, &QToolButton::clicked,
            tablaDesign, &TablaCentralWidget::establecerPK);

    // Mostrar ribbon de Inicio
    mostrarRibbonInicio();
}

void BaseDatosWindow::guardarTablasAbiertas()
{
    for (int i = 0; i < zonaCentral->count(); ++i) {
        QWidget *tabWidget = zonaCentral->widget(i);
        if (tabWidget && tabWidget != zonaCentral->widget(0)) {
            QString nombreTabla = zonaCentral->tabText(i);
            TablaCentralWidget *tablaDesign = tabWidget->property("tablaDesign").value<TablaCentralWidget*>();
            if (tablaDesign) {
                Metadata meta(nombreTabla);
                meta.campos = tablaDesign->obtenerCampos();
                meta.guardar();
            }
        }
    }
}
