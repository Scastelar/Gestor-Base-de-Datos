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
#include <QTabWidget>
#include <QComboBox>
#include <QToolButton>
#include <QMenuBar>
#include <QMenu>
#include <QActionGroup>
#include <QStackedWidget>
#include <QFrame>
#include <QHBoxLayout>
#include <QWidgetAction>

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
    listaTablas->addItem(new QListWidgetItem(iconTabla, "Tabla 1"));
    listaTablas->setIconSize(QSize(20, 20));

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

    // Zona central - Solo para mostrar tablas
    zonaCentral = new QStackedWidget();
    QLabel *welcomeLabel = new QLabel("<center><h2>Bienvenido a Base de Datos</h2>"
                                      "<p>Selecciona una tabla de la izquierda para comenzar</p></center>");
    welcomeLabel->setAlignment(Qt::AlignCenter);
    zonaCentral->addWidget(welcomeLabel);

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
}

void BaseDatosWindow::crearMenus()
{
    // Crear menús Archivo y Ayuda
    QMenu *menuArchivo = menuBar()->addMenu("Archivo");
    menuArchivo->addAction(QIcon(":/imgs/new.png"), "Nueva Base de Datos");
    menuArchivo->addAction(QIcon(":/imgs/open.png"), "Abrir");
    menuArchivo->addAction(QIcon(":/imgs/save.png"), "Guardar");
    menuArchivo->addAction(QIcon(":/imgs/save-as.png"), "Guardar Como");
    menuArchivo->addSeparator();
    menuArchivo->addAction(QIcon(":/imgs/exit.png"), "Salir");

    QMenu *menuAyuda = menuBar()->addMenu("Ayuda");
    menuAyuda->addAction(QIcon(":/imgs/help.png"), "Ayuda");
    menuAyuda->addAction(QIcon(":/imgs/about.png"), "Acerca de");

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
    QComboBox *comboVista = new QComboBox();
    comboVista->addItem(QIcon(":/imgs/design-view.png"), "Vista Diseño");
    comboVista->addItem(QIcon(":/imgs/datasheet-view.png"), "Vista Hoja de Datos");
    comboVista->setStyleSheet("QComboBox { height: 50px; }");
    connect(comboVista, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
        vistaHojaDatos = (index == 1);
        mostrarRibbonInicio();
    });

    vistaLayout->addWidget(vistaTitle);
    vistaLayout->addWidget(comboVista);
    vistaFrame->setLayout(vistaLayout);

    // Sección Llave Primaria
    QFrame *primaryKeyFrame = new QFrame();
    primaryKeyFrame->setFrameStyle(QFrame::Box);
    primaryKeyFrame->setStyleSheet(".ribbonSection { background: white; }");
    QVBoxLayout *primaryKeyLayout = new QVBoxLayout();

    QLabel *primaryKeyTitle = new QLabel("Clave");
    primaryKeyTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    QToolButton *btnLlavePrimaria = new QToolButton();
    btnLlavePrimaria->setIcon(QIcon(":/imgs/primary-key.png")); // Asegúrate de tener este icono
    btnLlavePrimaria->setText("Llave Primaria");
    btnLlavePrimaria->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnLlavePrimaria->setStyleSheet(".ribbonButton");

    primaryKeyLayout->addWidget(primaryKeyTitle);
    primaryKeyLayout->addWidget(btnLlavePrimaria);
    primaryKeyFrame->setLayout(primaryKeyLayout);

    // Sección Filas - Con VLayout para Insertar y Eliminar
    QFrame *filasFrame = new QFrame();
    filasFrame->setFrameStyle(QFrame::Box);
    filasFrame->setStyleSheet(".ribbonSection { background: white; }");
    QVBoxLayout *filasLayout = new QVBoxLayout();

    QLabel *filasTitle = new QLabel("Filas");
    filasTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    // Widget contenedor para los botones con VLayout
    QWidget *botonesFilasWidget = new QWidget();
    QVBoxLayout *botonesFilasLayout = new QVBoxLayout();
    botonesFilasLayout->setSpacing(5);
    botonesFilasLayout->setContentsMargins(0, 0, 0, 0);

    QToolButton *btnInsertarFila = new QToolButton();
    btnInsertarFila->setIcon(QIcon(":/imgs/insert-row.png")); // Asegúrate de tener este icono
    btnInsertarFila->setText("Insertar Fila");
    btnInsertarFila->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnInsertarFila->setStyleSheet(".ribbonButton");

    QToolButton *btnEliminarFila = new QToolButton();
    btnEliminarFila->setIcon(QIcon(":/imgs/delete-row.png")); // Asegúrate de tener este icono
    btnEliminarFila->setText("Eliminar Fila");
    btnEliminarFila->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnEliminarFila->setStyleSheet(".ribbonButton");

    botonesFilasLayout->addWidget(btnInsertarFila);
    botonesFilasLayout->addWidget(btnEliminarFila);
    botonesFilasWidget->setLayout(botonesFilasLayout);

    filasLayout->addWidget(filasTitle);
    filasLayout->addWidget(botonesFilasWidget);
    filasFrame->setLayout(filasLayout);

    // Sección Relaciones
    QFrame *relacionesFrame = new QFrame();
    relacionesFrame->setFrameStyle(QFrame::Box);
    relacionesFrame->setStyleSheet(".ribbonSection { background: white; }");
    QVBoxLayout *relacionesLayout = new QVBoxLayout();

    QLabel *relacionesTitle = new QLabel("Relaciones");
    relacionesTitle->setStyleSheet("QLabel { font-weight: bold; color: #2b579a; }");

    QToolButton *btnRelaciones = new QToolButton();
    btnRelaciones->setIcon(QIcon(":/imgs/relationships.png")); // Asegúrate de tener este icono
    btnRelaciones->setText("Relaciones");
    btnRelaciones->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    btnRelaciones->setStyleSheet(".ribbonButton");

    relacionesLayout->addWidget(relacionesTitle);
    relacionesLayout->addWidget(btnRelaciones);
    relacionesFrame->setLayout(relacionesLayout);

    // Agregar las secciones en el orden especificado
    inicioLayout->addWidget(vistaFrame);
    inicioLayout->addWidget(primaryKeyFrame);
    inicioLayout->addWidget(filasFrame);
    inicioLayout->addWidget(relacionesFrame);
    inicioLayout->addStretch();

    inicioWidget->setLayout(inicioLayout);
    ribbonInicio->addWidget(inicioWidget);

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

    QToolButton *btnDisenoTabla = new QToolButton();
    btnDisenoTabla->setIcon(QIcon(":/imgs/table-design.png"));
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
    btnDisenoConsulta->setIcon(QIcon(":/imgs/query-design.png"));
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
    btnDisenoReporte->setIcon(QIcon(":/imgs/report-design.png"));
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

    // Actualizar el estado de los botones
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
    // Aquí puedes cambiar el icono del botón de filtro si lo deseas
}

void BaseDatosWindow::abrirTabla(QListWidgetItem *item)
{
    QWidget *tablaDesign = new TablaCentralWidget();
    zonaCentral->addWidget(tablaDesign);
    zonaCentral->setCurrentWidget(tablaDesign);

    // Mostrar ribbon de Inicio al abrir una tabla
    mostrarRibbonInicio();
}
