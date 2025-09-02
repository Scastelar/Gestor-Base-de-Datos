#include "basedatoswindow.h"
#include "datasheetwidget.h"
#include "relacioneswidget.h"
#include "metadata.h"

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


BaseDatosWindow::BaseDatosWindow(QWidget *parent)
    : QMainWindow(parent), vistaHojaDatos(false), filtroActivo(false)
{
    QIcon iconTabla(":/imgs/table.png");
    QIcon icon(":/imgs/access.png");
    this->setWindowIcon(icon);

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

    // Zona central - QTabWidget en lugar de QStackedWidget
    zonaCentral = new QTabWidget();
    zonaCentral->setTabsClosable(true); // Permitir cerrar tabs
    zonaCentral->setMovable(true); // Permitir mover tabs

    // Conectar señal de cierre de tab
    connect(zonaCentral, &QTabWidget::tabCloseRequested, this, &BaseDatosWindow::cerrarTab);

    // Widget de bienvenida inicial
    QLabel *welcomeLabel = new QLabel("<center><h2>Bienvenido a Base de Datos</h2>"
                                      "<p>Selecciona una tabla de la izquierda para comenzar</p></center>");
    welcomeLabel->setAlignment(Qt::AlignCenter);

    // Crear un widget contenedor para el mensaje de bienvenida
    QWidget *welcomeWidget = new QWidget();
    QVBoxLayout *welcomeLayout = new QVBoxLayout(welcomeWidget);
    welcomeLayout->addWidget(welcomeLabel);
    welcomeWidget->setLayout(welcomeLayout);

    // Añadir el widget de bienvenida como primera pestaña
    zonaCentral->addTab(welcomeWidget, "Inicio");
    zonaCentral->setTabEnabled(0, false); // Deshabilitar la pestaña de inicio

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
    ribbonInicio->setStyleSheet(styleSheet);
    ribbonCrear->setStyleSheet(styleSheet);

    // Mostrar ribbon inicial (Inicio)
    mostrarRibbonInicio();
    addToolBar(Qt::TopToolBarArea, ribbonInicio);
    ribbonCrear->setVisible(false);

    connect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        vistaHojaDatos = (index == 1);
        cambiarVista(); // Llamar al método que cambia la vista
    });
}

void BaseDatosWindow::crearMenus()
{
    // Crear menús en el orden correcto: Archivo primero, Ayuda último
    QMenu *menuArchivo = menuBar()->addMenu("Archivo");
    menuArchivo->addAction(QIcon(":/imgs/new.png"), "Nueva Base de Datos");
    menuArchivo->addAction(QIcon(":/imgs/open.png"), "Abrir");
    menuArchivo->addAction(QIcon(":/imgs/save.png"), "Guardar");
    menuArchivo->addAction(QIcon(":/imgs/save-as.png"), "Guardar Como");
    menuArchivo->addSeparator();
    menuArchivo->addAction(QIcon(":/imgs/exit.png"), "Salir");

    // Crear un widget personalizado para los botones del ribbon
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

    // Agregar botones al layout
    ribbonTabLayout->addWidget(btnInicio);
    ribbonTabLayout->addWidget(btnCrear);
    ribbonTabLayout->addStretch();

    // Agregar el widget de pestañas al menú como un widget de esquina
    menuBar()->setCornerWidget(ribbonTabWidget, Qt::TopLeftCorner);

    // Crear menú Ayuda al final
    QMenu *menuAyuda = menuBar()->addMenu("Ayuda");
    menuAyuda->addAction(QIcon(":/imgs/help.png"), "Ayuda");
    menuAyuda->addAction(QIcon(":/imgs/about.png"), "Acerca de");
}

void BaseDatosWindow::crearToolbars()
{
    // Ribbon Inicio
    ribbonInicio = new QToolBar("Ribbon Inicio", this);
    ribbonInicio->setMovable(false);
    ribbonInicio->setIconSize(QSize(16, 16));

    QWidget *inicioWidget = new QWidget();
    QHBoxLayout *inicioLayout = new QHBoxLayout();
    inicioLayout->setSpacing(10);
    inicioLayout->setContentsMargins(15, 5, 15, 5);

    // Sección Vista - Cambiada a ComboBox
    QFrame *vistaFrame = new QFrame();
    vistaFrame->setFrameStyle(QFrame::Box);
    vistaFrame->setStyleSheet(".ribbonSection { background: white; }");
    QVBoxLayout *vistaLayout = new QVBoxLayout();

    QLabel *vistaTitle = new QLabel("Vista");
    vistaTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    // ComboBox para cambiar entre vistas
    comboVista = new QComboBox(); // Hacerlo miembro de la clase para poder acceder desde otros métodos
    comboVista->addItem(QIcon(":/imgs/design-view.png"), "Vista Diseño");
    comboVista->addItem(QIcon(":/imgs/datasheet-view.png"), "Vista Hoja de Datos");
    comboVista->setStyleSheet("QComboBox { height: 50px; }");
    connect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        vistaHojaDatos = (index == 1);
        mostrarRibbonInicio(); // Actualizar la interfaz cuando cambia la vista
    });

    vistaLayout->addWidget(vistaTitle);
    vistaLayout->addWidget(comboVista);
    vistaFrame->setLayout(vistaLayout);

    // Sección Filtros (solo visible en Vista Hoja de Datos)
    QFrame *filtrosFrame = new QFrame();
    filtrosFrame->setFrameStyle(QFrame::Box);
    filtrosFrame->setStyleSheet(".ribbonSection { background: white; }");
    QVBoxLayout *filtrosLayout = new QVBoxLayout();

    QLabel *filtrosTitle = new QLabel("Filtros");
    filtrosTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    btnFiltrar = new QToolButton();
    btnFiltrar->setIcon(QIcon(":/imgs/filter.png"));
    btnFiltrar->setText("Filtrar");
    btnFiltrar->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnFiltrar->setStyleSheet(".ribbonButton");

    filtrosLayout->addWidget(filtrosTitle);
    filtrosLayout->addWidget(btnFiltrar);
    filtrosFrame->setLayout(filtrosLayout);
    filtrosFrame->setVisible(false); // Oculto por defecto

    // Sección Orden (solo visible en Vista Hoja de Datos)
    QFrame *ordenFrame = new QFrame();
    ordenFrame->setFrameStyle(QFrame::Box);
    ordenFrame->setStyleSheet(".ribbonSection { background: white; }");
    QHBoxLayout *ordenLayout = new QHBoxLayout();

    QLabel *ordenTitle = new QLabel("Orden");
    ordenTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    // Widget contenedor para los botones de orden con VLayout
    QWidget *botonesOrdenWidget = new QWidget();
    QHBoxLayout *botonesOrdenLayout = new QHBoxLayout();
    botonesOrdenLayout->setSpacing(5);
    botonesOrdenLayout->setContentsMargins(0, 0, 0, 0);

    btnAscendente = new QToolButton();
    btnAscendente->setIcon(QIcon(":/imgs/sort-asc.png"));
    btnAscendente->setText("Ascendente");
    btnAscendente->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnAscendente->setStyleSheet(".ribbonButton");

    btnDescendente = new QToolButton();
    btnDescendente->setIcon(QIcon(":/imgs/sort-desc.png"));
    btnDescendente->setText("Descendente");
    btnDescendente->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnDescendente->setStyleSheet(".ribbonButton");

    botonesOrdenLayout->addWidget(btnAscendente);
    botonesOrdenLayout->addWidget(btnDescendente);
    botonesOrdenWidget->setLayout(botonesOrdenLayout);

    ordenLayout->addWidget(ordenTitle);
    ordenLayout->addWidget(botonesOrdenWidget);
    ordenFrame->setLayout(ordenLayout);
    ordenFrame->setVisible(false); // Oculto por defecto

    // Separador (solo visible en Vista Hoja de Datos)
    QFrame *separador = new QFrame();
    separador->setFrameShape(QFrame::VLine);
    separador->setFrameShadow(QFrame::Sunken);
    separador->setStyleSheet("background-color: #d0d0d0;");
    separador->setVisible(false); // Oculto por defecto

    // Sección Llave Primaria (solo visible en Vista Diseño)
    QFrame *primaryKeyFrame = new QFrame();
    primaryKeyFrame->setFrameStyle(QFrame::Box);
    primaryKeyFrame->setStyleSheet(".ribbonSection { background: white; }");
    QVBoxLayout *primaryKeyLayout = new QVBoxLayout();

    QLabel *primaryKeyTitle = new QLabel("Clave");
    primaryKeyTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    btnLlavePrimaria = new QToolButton();
    btnLlavePrimaria->setIcon(QIcon(":/imgs/key.png"));
    btnLlavePrimaria->setText("Llave Primaria");
    btnLlavePrimaria->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnLlavePrimaria->setStyleSheet(".ribbonButton");

    primaryKeyLayout->addWidget(primaryKeyTitle);
    primaryKeyLayout->addWidget(btnLlavePrimaria);
    primaryKeyFrame->setLayout(primaryKeyLayout);

    // Sección Filas (visible en ambas vistas pero con layout diferente)
    QFrame *filasFrame = new QFrame();
    filasFrame->setFrameStyle(QFrame::Box);
    filasFrame->setStyleSheet(".ribbonSection { background: white; }");
    filasLayout = new QHBoxLayout(); // Hacerlo miembro para poder cambiar entre V y H

    QLabel *filasTitle = new QLabel("Filas");
    filasTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    // Widget contenedor para los botones con layout dinámico
    botonesFilasWidget = new QWidget();
    botonesFilasVLayout = new QVBoxLayout(); // Layout vertical
    botonesFilasHLayout = new QHBoxLayout(); // Layout horizontal
    botonesFilasVLayout->setSpacing(5);
    botonesFilasVLayout->setContentsMargins(0, 0, 0, 0);
    botonesFilasHLayout->setSpacing(5);
    botonesFilasHLayout->setContentsMargins(0, 0, 0, 0);

    btnInsertarFila = new QToolButton();
    btnInsertarFila->setIcon(QIcon(":/imgs/insert-row.png"));
    btnInsertarFila->setText("Insertar Fila");
    btnInsertarFila->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnInsertarFila->setStyleSheet(".ribbonButton");

    btnEliminarFila = new QToolButton();
    btnEliminarFila->setIcon(QIcon(":/imgs/delete-row.png"));
    btnEliminarFila->setText("Eliminar Fila");
    btnEliminarFila->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnEliminarFila->setStyleSheet(".ribbonButton");

    // Agregar botones a ambos layouts
    botonesFilasVLayout->addWidget(btnInsertarFila);
    botonesFilasVLayout->addWidget(btnEliminarFila);
    botonesFilasHLayout->addWidget(btnInsertarFila);
    botonesFilasHLayout->addWidget(btnEliminarFila);

    // Usar layout vertical por defecto
    botonesFilasWidget->setLayout(botonesFilasHLayout);

    filasLayout->addWidget(filasTitle);
    filasLayout->addWidget(botonesFilasWidget);
    filasFrame->setLayout(filasLayout);

    // Sección Relaciones (solo visible en Vista Diseño)
    QFrame *relacionesFrame = new QFrame();
    relacionesFrame->setFrameStyle(QFrame::Box);
    relacionesFrame->setStyleSheet(".ribbonSection { background: white; }");
    QVBoxLayout *relacionesLayout = new QVBoxLayout();

    QLabel *relacionesTitle = new QLabel("Relaciones");
    relacionesTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    btnRelaciones = new QToolButton();
    btnRelaciones->setIcon(QIcon(":/imgs/relationships.png"));
    btnRelaciones->setText("Relaciones");
    btnRelaciones->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnRelaciones->setStyleSheet(".ribbonButton");
    connect(btnRelaciones, &QToolButton::clicked, this, &BaseDatosWindow::abrirRelaciones);

    relacionesLayout->addWidget(relacionesTitle);
    relacionesLayout->addWidget(btnRelaciones);
    relacionesFrame->setLayout(relacionesLayout);

    // Agregar las secciones en el orden especificado
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

    // Guardar referencias a los widgets para poder mostrarlos/ocultarlos
    seccionesVistaHojaDatos << filtrosFrame << ordenFrame << separador;
    seccionesVistaDiseno << primaryKeyFrame << relacionesFrame;

    // Ribbon Crear - Cambiar layouts a horizontal
    ribbonCrear = new QToolBar("Ribbon Crear", this);
    ribbonCrear->setMovable(false);
    ribbonCrear->setIconSize(QSize(16, 16));

    QWidget *crearWidget = new QWidget();
    QHBoxLayout *crearLayout = new QHBoxLayout();
    crearLayout->setSpacing(10);
    crearLayout->setContentsMargins(15, 5, 15, 5);

    // Sección Tablas - Cambiada a horizontal
    QFrame *tablasFrame = new QFrame();
    tablasFrame->setFrameStyle(QFrame::Box);
    tablasFrame->setStyleSheet(".ribbonSection { background: white; }");
    QHBoxLayout *tablasLayout = new QHBoxLayout(); // Cambiado a horizontal

    QLabel *tablasTitle = new QLabel("Tablas");
    tablasTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    QToolButton *btnTabla = new QToolButton();
    btnTabla->setIcon(QIcon(":/imgs/table.png"));
    btnTabla->setText("Tabla");
    btnTabla->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnTabla->setStyleSheet(".ribbonButton");
    connect(btnTabla, &QToolButton::clicked, this, &BaseDatosWindow::crearNuevaTabla);


    QToolButton *btnDisenoTabla = new QToolButton();
    btnDisenoTabla->setIcon(QIcon(":/imgs/form-design.png"));
    btnDisenoTabla->setText("Diseño");
    btnDisenoTabla->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnDisenoTabla->setStyleSheet(".ribbonButton");

    tablasLayout->addWidget(tablasTitle);
    tablasLayout->addWidget(btnTabla);
    tablasLayout->addWidget(btnDisenoTabla);
    tablasFrame->setLayout(tablasLayout);

    // Sección Consultas - Cambiada a horizontal
    QFrame *queriesFrame = new QFrame();
    queriesFrame->setFrameStyle(QFrame::Box);
    queriesFrame->setStyleSheet(".ribbonSection { background: white; }");
    QHBoxLayout *queriesLayout = new QHBoxLayout(); // Cambiado a horizontal

    QLabel *queriesTitle = new QLabel("Consultas");
    queriesTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    QToolButton *btnConsulta = new QToolButton();
    btnConsulta->setIcon(QIcon(":/imgs/query.png"));
    btnConsulta->setText("Consulta");
    btnConsulta->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnConsulta->setStyleSheet(".ribbonButton");

    QToolButton *btnDisenoConsulta = new QToolButton();
    btnDisenoConsulta->setIcon(QIcon(":/imgs/form-design.png"));
    btnDisenoConsulta->setText("Diseño");
    btnDisenoConsulta->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnDisenoConsulta->setStyleSheet(".ribbonButton");

    queriesLayout->addWidget(queriesTitle);
    queriesLayout->addWidget(btnConsulta);
    queriesLayout->addWidget(btnDisenoConsulta);
    queriesFrame->setLayout(queriesLayout);

    // Sección Formularios - Cambiada a horizontal
    QFrame *formsFrame = new QFrame();
    formsFrame->setFrameStyle(QFrame::Box);
    formsFrame->setStyleSheet(".ribbonSection { background: white; }");
    QHBoxLayout *formsLayout = new QHBoxLayout(); // Cambiado a horizontal

    QLabel *formsTitle = new QLabel("Formularios");
    formsTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    QToolButton *btnFormulario = new QToolButton();
    btnFormulario->setIcon(QIcon(":/imgs/form.png"));
    btnFormulario->setText("Formulario");
    btnFormulario->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnFormulario->setStyleSheet(".ribbonButton");

    QToolButton *btnDisenoFormulario = new QToolButton();
    btnDisenoFormulario->setIcon(QIcon(":/imgs/form-design.png"));
    btnDisenoFormulario->setText("Diseño");
    btnDisenoFormulario->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnDisenoFormulario->setStyleSheet(".ribbonButton");

    formsLayout->addWidget(formsTitle);
    formsLayout->addWidget(btnFormulario);
    formsLayout->addWidget(btnDisenoFormulario);
    formsFrame->setLayout(formsLayout);

    // Sección Reportes - Cambiada a horizontal
    QFrame *reportsFrame = new QFrame();
    reportsFrame->setFrameStyle(QFrame::Box);
    reportsFrame->setStyleSheet(".ribbonSection { background: white; }");
    QHBoxLayout *reportsLayout = new QHBoxLayout(); // Cambiado a horizontal

    QLabel *reportsTitle = new QLabel("Reportes");
    reportsTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    QToolButton *btnReporte = new QToolButton();
    btnReporte->setIcon(QIcon(":/imgs/report.png"));
    btnReporte->setText("Reporte");
    btnReporte->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnReporte->setStyleSheet(".ribbonButton");

    QToolButton *btnDisenoReporte = new QToolButton();
    btnDisenoReporte->setIcon(QIcon(":/imgs/form-design.png"));
    btnDisenoReporte->setText("Diseño");
    btnDisenoReporte->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnDisenoReporte->setStyleSheet(".ribbonButton");

    reportsLayout->addWidget(reportsTitle);
    reportsLayout->addWidget(btnReporte);
    reportsLayout->addWidget(btnDisenoReporte);
    reportsFrame->setLayout(reportsLayout);

    crearLayout->addWidget(tablasFrame);
    crearLayout->addWidget(queriesFrame);
    crearLayout->addWidget(formsFrame);
    crearLayout->addWidget(reportsFrame);
    crearLayout->addStretch();

    crearWidget->setLayout(crearLayout);
    ribbonCrear->addWidget(crearWidget);
}

void BaseDatosWindow::mostrarRibbonInicio()
{
    // Ocultar todos los ribbons primero
    if (ribbonCrear->isVisible()) {
        removeToolBar(ribbonCrear);
        ribbonCrear->setVisible(false);
    }

    // Mostrar el ribbon de Inicio si no está visible
    if (!ribbonInicio->isVisible()) {
        addToolBar(Qt::TopToolBarArea, ribbonInicio);
        ribbonInicio->setVisible(true);
    }

    // Usar el estado actual del combo box en lugar de la variable vistaHojaDatos
    bool esHojaDatos = (comboVista->currentIndex() == 1);

    // Mostrar/ocultar secciones según la vista
    foreach(QFrame *frame, seccionesVistaHojaDatos) {
        frame->setVisible(esHojaDatos);
    }

    foreach(QFrame *frame, seccionesVistaDiseno) {
        frame->setVisible(!esHojaDatos);
    }

    // Cambiar layout de botones de filas según la vista
    if (esHojaDatos) {
        // En Vista Hoja de Datos: layout horizontal
        botonesFilasWidget->setLayout(botonesFilasHLayout);
    } else {
        // En Vista Diseño: layout vertical
        botonesFilasWidget->setLayout(botonesFilasVLayout);

        // Actualizar propiedades para la vista diseño si existe una tabla abierta
        QWidget *currentTab = zonaCentral->currentWidget();
        if (currentTab && currentTab != zonaCentral->widget(0)) { // No es la pestaña de inicio
            // Obtener la vista diseño de la tabla actual
            TablaCentralWidget *tablaDesign = currentTab->property("tablaDesign").value<TablaCentralWidget*>();
            if (tablaDesign) {
                tablaDesign->actualizarPropiedades();
            }
        }
    }

    // Actualizar el estado de los botones en el corner widget
    QWidget *cornerWidget = menuBar()->cornerWidget(Qt::TopLeftCorner);
    if (cornerWidget) {
        QList<QToolButton*> buttons = cornerWidget->findChildren<QToolButton*>();
        if (buttons.size() >= 2) {
            buttons[0]->setChecked(true);  // Botón Inicio
            buttons[1]->setChecked(false); // Botón Crear
        }
    }
}

void BaseDatosWindow::mostrarRibbonCrear()
{
    // Ocultar todos los ribbons primero
    if (ribbonInicio->isVisible()) {
        removeToolBar(ribbonInicio);
        ribbonInicio->setVisible(false);
    }

    // Mostrar el ribbon de Crear si no está visible
    if (!ribbonCrear->isVisible()) {
        addToolBar(Qt::TopToolBarArea, ribbonCrear);
        ribbonCrear->setVisible(true);
    }

    // Actualizar el estado de los botones
    QWidget *cornerWidget = menuBar()->cornerWidget(Qt::TopLeftCorner);
    if (cornerWidget) {
        QList<QToolButton*> buttons = cornerWidget->findChildren<QToolButton*>();
        if (buttons.size() >= 2) {
            buttons[0]->setChecked(false); // Botón Inicio
            buttons[1]->setChecked(true);  // Botón Crear
        }
    }
}

void BaseDatosWindow::toggleFiltro()
{
    filtroActivo = !filtroActivo;
}

int BaseDatosWindow::encontrarTablaEnTabs(const QString &nombreTabla)
{
    for (int i = 0; i < zonaCentral->count(); ++i) {
        if (zonaCentral->tabText(i) == nombreTabla) {
            return i;
        }
    }
    return -1; // No encontrado
}

//VISTA DISENO
void BaseDatosWindow::abrirTabla(QListWidgetItem *item)
{
    tablaActualNombre = item->text();

    // Cargar metadata
    Metadata meta = Metadata::cargar(QDir::currentPath() + "/tables/" + tablaActualNombre + ".meta");

    // Verificar si la tabla ya está abierta en algún tab
    int tabIndex = encontrarTablaEnTabs(tablaActualNombre);

    if (tabIndex != -1) {
        // La tabla ya está abierta, simplemente cambiar a esa pestaña
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

    // Añadir ambas vistas al stacked widget
    tablaStacked->addWidget(tablaDesign);
    tablaStacked->addWidget(tablaDataSheet);

    // Configurar la vista inicial según el modo actual (sincronizado con comboVista)
    if (comboVista->currentIndex() == 1) { // Usar el valor actual del combo box
        tablaStacked->setCurrentWidget(tablaDataSheet);
    } else {
        tablaStacked->setCurrentWidget(tablaDesign);
        tablaDesign->cargarCampos(meta.campos);

    }

    containerLayout->addWidget(tablaStacked);

    // Añadir el nuevo tab
    int newTabIndex = zonaCentral->addTab(tablaContainer, tablaActualNombre);
    zonaCentral->setCurrentIndex(newTabIndex);

    // Almacenar referencias a las vistas en propiedades del widget contenedor
    tablaContainer->setProperty("tablaDesign", QVariant::fromValue(tablaDesign));
    tablaContainer->setProperty("tablaDataSheet", QVariant::fromValue(tablaDataSheet));
    tablaContainer->setProperty("tablaStacked", QVariant::fromValue(tablaStacked));

    // Conectar señales según la vista actual
    disconnect(btnLlavePrimaria, &QToolButton::clicked, 0, 0);
    if (comboVista->currentIndex() == 1) {
        connect(btnLlavePrimaria, &QToolButton::clicked,
                tablaDataSheet, &DataSheetWidget::establecerPK);
    } else {
        connect(btnLlavePrimaria, &QToolButton::clicked,
                tablaDesign, &TablaCentralWidget::establecerPK);
    }

    // Mostrar ribbon de Inicio al abrir una tabla
    mostrarRibbonInicio();
}

void BaseDatosWindow::cambiarVista()
{
    // Obtener el widget actual (tabla abierta)
    QWidget *currentTab = zonaCentral->currentWidget();
    if (!currentTab || currentTab == zonaCentral->widget(0)) return; // No hay tabla abierta o es la pestaña de inicio

    // Obtener el stacked widget y las vistas de la tabla actual
    QStackedWidget *tablaStacked = currentTab->property("tablaStacked").value<QStackedWidget*>();
    TablaCentralWidget *tablaDesign = currentTab->property("tablaDesign").value<TablaCentralWidget*>();
    DataSheetWidget *tablaDataSheet = currentTab->property("tablaDataSheet").value<DataSheetWidget*>();

    if (!tablaStacked || !tablaDesign || !tablaDataSheet) return;

    // Desconectar señales temporariamente para evitar loops infinitos
    disconnect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), 0, 0);

    // Sincronizar el combo box con el estado actual
    if (vistaHojaDatos) {
        comboVista->setCurrentIndex(1); // Vista Hoja de Datos
        tablaStacked->setCurrentWidget(tablaDataSheet);
    } else {
        comboVista->setCurrentIndex(0); // Vista Diseño
        tablaStacked->setCurrentWidget(tablaDesign);
    }

    if (tablaDesign && !tablaActualNombre.isEmpty()) {
        Metadata meta(tablaActualNombre);
        meta.campos = tablaDesign->obtenerCampos();
        meta.guardar();
    }

    // Reconectar la señal
    connect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        vistaHojaDatos = (index == 1);
        cambiarVista();
    });

    // Desconectar y reconectar señales del botón de llave primaria
    disconnect(btnLlavePrimaria, &QToolButton::clicked, 0, 0);
    if (vistaHojaDatos) {
        connect(btnLlavePrimaria, &QToolButton::clicked,
                tablaDataSheet, &DataSheetWidget::establecerPK);
    } else {
        connect(btnLlavePrimaria, &QToolButton::clicked,
                tablaDesign, &TablaCentralWidget::establecerPK);

        // Actualizar propiedades para la vista diseño
        tablaDesign->actualizarPropiedades();
    }

    // Actualizar la interfaz
    mostrarRibbonInicio();
}



void BaseDatosWindow::transferirDatosVista(QWidget *origen, QWidget *destino)
{
    // Esta es una implementación básica - necesitarás adaptarla según tus necesidades
    if (TablaCentralWidget *origenDesign = qobject_cast<TablaCentralWidget*>(origen)) {
        if (DataSheetWidget *destinoDataSheet = qobject_cast<DataSheetWidget*>(destino)) {
            // Transferir datos de diseño a hoja de datos
            // Implementar según la estructura de tus datos
        }
    } else if (DataSheetWidget *origenDataSheet = qobject_cast<DataSheetWidget*>(origen)) {
        if (TablaCentralWidget *destinoDesign = qobject_cast<TablaCentralWidget*>(destino)) {
            // Transferir datos de hoja de datos a diseño
            // Implementar según la estructura de tus datos
        }
    }
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

    // Mostrar ribbon de Inicio
    mostrarRibbonInicio();

    qDebug() << "Ventana de relaciones abierta en nueva pestaña";
}

void BaseDatosWindow::cerrarRelacionesYVolver()
{
    // Buscar la pestaña de relaciones
    for (int i = 0; i < zonaCentral->count(); ++i) {
        QWidget *tabWidget = zonaCentral->widget(i);
        if (qobject_cast<RelacionesWidget*>(tabWidget)) {
            // Cerrar la pestaña de relaciones
            zonaCentral->removeTab(i);
            break;
        }
    }

    qDebug() << "Ventana de relaciones cerrada";
}

void BaseDatosWindow::cerrarTab(int index)
{
    // No permitir cerrar la pestaña de inicio
    if (index == 0 && !zonaCentral->isTabEnabled(0)) {
        return;
    }

    // Remover la pestaña (esto eliminará el widget automáticamente)
    zonaCentral->removeTab(index);

    // Si no hay más pestañas (solo queda la de inicio), asegurarse de que esté visible
    if (zonaCentral->count() == 1 && !zonaCentral->isTabEnabled(0)) {
        zonaCentral->setCurrentIndex(0);
    }
}

void BaseDatosWindow::crearNuevaTabla() {
    // Pide un nombre
    bool ok;
    QString nombreTabla = QInputDialog::getText(this, "Nueva Tabla",
                                                "Nombre de la tabla:",
                                                QLineEdit::Normal,
                                                "TablaNueva", &ok);
    if (!ok || nombreTabla.isEmpty()) return;

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
    disconnect(btnLlavePrimaria, &QToolButton::clicked, 0, 0);
    connect(btnLlavePrimaria, &QToolButton::clicked,
            tablaDesign, &TablaCentralWidget::establecerPK);

    // Mostrar ribbon de Inicio
    mostrarRibbonInicio();
}

BaseDatosWindow::~BaseDatosWindow() {
    // Guardar los datos de todas las tablas abiertas
    for (int i = 0; i < zonaCentral->count(); ++i) {
        QWidget *tabWidget = zonaCentral->widget(i);
        if (tabWidget && tabWidget != zonaCentral->widget(0)) { // No es la pestaña de inicio
            QString nombreTabla = zonaCentral->tabText(i);

            // Obtener la vista diseño de la tabla
            TablaCentralWidget *tablaDesign = tabWidget->property("tablaDesign").value<TablaCentralWidget*>();
            if (tablaDesign) {
                Metadata meta(nombreTabla);
                meta.campos = tablaDesign->obtenerCampos();
                meta.guardar();
            }
        }
    }
}
