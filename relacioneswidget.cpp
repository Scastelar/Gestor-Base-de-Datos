#include "relacioneswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QAction>
#include <QListWidget>
#include <QGroupBox>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QFrame>
#include <QScrollArea>
#include <QLabel>
#include <QCloseEvent>

RelacionesWidget::RelacionesWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #ffffff; color: #000000;");
    crearToolbar();
    crearLayoutPrincipal();
}

void RelacionesWidget::crearToolbar()
{
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));
    toolbar->setStyleSheet(
        "QToolBar {"
        "   background-color: #f0f0f0;"
        "   border-bottom: 1px solid #cccccc;"
        "   padding: 2px;"
        "}"
        "QToolButton {"
        "   padding: 4px 8px;"
        "   border: 1px solid transparent;"
        "   border-radius: 3px;"
        "   background: transparent;"
        "   color: #000000;"
        "}"
        "QToolButton:hover {"
        "   background-color: #e0e0e0;"
        "   border: 1px solid #cccccc;"
        "}"
        );

    // Botón Editar (lo mantenemos)
    QToolButton *btnEditar = new QToolButton();
    btnEditar->setText("Editar");
    toolbar->addWidget(btnEditar);

    toolbar->addSeparator();

    // Botón Cerrar
    QToolButton *btnCerrar = new QToolButton();
    btnCerrar->setText("Cerrar");
    btnCerrar->setStyleSheet("QToolButton { color: #000000; font-weight: bold; }");
    toolbar->addWidget(btnCerrar);

    // 🔹 Conectar al método close() del widget
    connect(btnCerrar, &QToolButton::clicked, this, &QWidget::close);


    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(toolbar);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
}

void RelacionesWidget::crearCardTabla(const QString &nombreTabla)
{
    QFrame *card = new QFrame();
    card->setStyleSheet(
        "QFrame {"
        "   background-color: #ffffff;"
        "   border: 2px solid #2b579a;"
        "   border-radius: 5px;"
        "   margin: 5px;"
        "   padding: 0px;"
        "   min-width: 180px;"
        "}"
        "QLabel {"
        "   color: #000000;"
        "   padding: 2px;"
        "}"
        );

    QVBoxLayout *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(0, 0, 0, 0);
    cardLayout->setSpacing(0);

    // Título de la tabla (header azul como en Access)
    QLabel *titulo = new QLabel(nombreTabla);
    titulo->setStyleSheet(
        "font-weight: bold; "
        "font-size: 12px; "
        "background-color: #2b579a; "
        "color: white; "
        "padding: 6px; "
        "border-top-left-radius: 3px; "
        "border-top-right-radius: 3px;"
        );
    titulo->setAlignment(Qt::AlignCenter);
    titulo->setMinimumHeight(25);

    // Contenedor para los campos
    QWidget *camposContainer = new QWidget();
    camposContainer->setStyleSheet("background-color: #ffffff;");
    QVBoxLayout *camposLayout = new QVBoxLayout(camposContainer);
    camposLayout->setContentsMargins(8, 4, 8, 4);
    camposLayout->setSpacing(2);

    // Campos de la tabla (simulados)
    QStringList campos;
    if (nombreTabla == "Customers") {
        campos = {"CustomerID", "FirstName", "LastName", "Email", "Phone"};
    } else if (nombreTabla == "Orders") {
        campos = {"OrderID", "CustomerID", "OrderDate", "TotalAmount"};
    } else if (nombreTabla == "Products") {
        campos = {"ProductID", "ProductName", "Price", "Category"};
    } else {
        campos = {"ID", "Nombre", "Apellido", "Email", "Teléfono"};
    }

    for (const QString &campo : campos) {
        QLabel *labelCampo = new QLabel("• " + campo);
        labelCampo->setStyleSheet("font-size: 11px; padding: 1px;");
        camposLayout->addWidget(labelCampo);
    }

    cardLayout->addWidget(titulo);
    cardLayout->addWidget(camposContainer);

    // Hacer la card seleccionable y movible (como en Access)
    card->setProperty("tablaNombre", nombreTabla);
    card->installEventFilter(this);

    // Agregar al layout de cards con alineación a la izquierda
    QHBoxLayout *cardContainerLayout = new QHBoxLayout();
    cardContainerLayout->addWidget(card);
    cardContainerLayout->setAlignment(Qt::AlignLeft);

    QWidget *cardContainer = new QWidget();
    cardContainer->setLayout(cardContainerLayout);

    cardsLayout->addWidget(cardContainer);
}

void RelacionesWidget::crearLayoutPrincipal()
{
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());

    QHBoxLayout *contentLayout = new QHBoxLayout();

    // Panel de tablas a la izquierda
    QFrame *panelTablasFrame = new QFrame();
    panelTablasFrame->setStyleSheet(
        "QFrame {"
        "   background-color: #f8f8f8;"
        "   border-right: 1px solid #cccccc;"
        "}"
        "QGroupBox {"
        "   font-weight: bold;"
        "   border: 1px solid #cccccc;"
        "   margin-top: 1ex;"
        "   background-color: #ffffff;"
        "}"
        "QGroupBox::title {"
        "   subcontrol-origin: margin;"
        "   subcontrol-position: top center;"
        "   padding: 0 5px;"
        "   color: #000000;"
        "}"
        "QListWidget {"
        "   background-color: #ffffff;"
        "   border: 1px solid #cccccc;"
        "   color: #000000;"
        "}"
        "QListWidget::item {"
        "   padding: 2px;"
        "}"
        "QListWidget::item:selected {"
        "   background-color: #2b579a;"
        "   color: #ffffff;"
        "}"
        "QPushButton {"
        "   background-color: #2b579a;"
        "   color: white;"
        "   border: none;"
        "   padding: 5px;"
        "   border-radius: 3px;"
        "   margin: 5px;"
        "}"
        "QPushButton:hover {"
        "   background-color: #1e3f6e;"
        "}"
        );

    QVBoxLayout *panelLayout = new QVBoxLayout(panelTablasFrame);
    panelLayout->setContentsMargins(5, 5, 5, 5);

    // Grupo de Tablas
    QGroupBox *grupoTablas = new QGroupBox("Tablas Disponibles");
    QVBoxLayout *tablasLayout = new QVBoxLayout(grupoTablas);

    QListWidget *listaTablas = new QListWidget();
    listaTablas->addItems({"Customers", "Orders", "Products", "Employees", "Suppliers"});
    tablasLayout->addWidget(listaTablas);

    // Botón Agregar Tabla
    QPushButton *btnAgregar = new QPushButton("Agregar Tabla");
    tablasLayout->addWidget(btnAgregar);
    connect(btnAgregar, &QPushButton::clicked, this, [this, listaTablas]() {
        QListWidgetItem *item = listaTablas->currentItem();
        if (item) {
            crearCardTabla(item->text());
        }
    });

    // Grupo de Consultas
    QGroupBox *grupoConsultas = new QGroupBox("Consultas");
    QVBoxLayout *consultasLayout = new QVBoxLayout(grupoConsultas);

    QListWidget *listaConsultas = new QListWidget();
    listaConsultas->addItems({"OrderDates", "SalesReport", "CustomerOrders"});
    consultasLayout->addWidget(listaConsultas);

    panelLayout->addWidget(grupoTablas);
    panelLayout->addWidget(grupoConsultas);
    panelLayout->addStretch();

    panelTablasFrame->setFixedWidth(250);

    // Área de relaciones a la derecha - Ahora con layout de flujo para cards
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setStyleSheet(
        "QScrollArea {"
        "   background-color: #e8e8e8;"
        "   border: none;"
        "}"
        "QScrollArea > QWidget > QWidget {"
        "   background-color: #e8e8e8;"
        "}"
        );

    QWidget *scrollContent = new QWidget();

    // Usar QVBoxLayout para alinear las cards verticalmente como en Access
    cardsLayout = new QVBoxLayout(scrollContent);
    cardsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    cardsLayout->setContentsMargins(15, 15, 15, 15);
    cardsLayout->setSpacing(15);

    scrollArea->setWidget(scrollContent);

    contentLayout->addWidget(panelTablasFrame);
    contentLayout->addWidget(scrollArea, 1);

    mainLayout->addLayout(contentLayout);
}

void RelacionesWidget::crearPanelTablas()
{
    // Ya implementado en crearLayoutPrincipal
}

void RelacionesWidget::crearAreaRelaciones()
{
    // Ya implementado en crearLayoutPrincipal
}

bool RelacionesWidget::eventFilter(QObject *obj, QEvent *event)
{
    // Aquí puedes implementar la lógica para hacer las cards movibles
    // y seleccionables como en Access
    return QWidget::eventFilter(obj, event);
}

void RelacionesWidget::closeEvent(QCloseEvent *event)
{
    emit cerrada(); // Emitir señal de que se está cerrando
    event->accept(); // Aceptar el evento de cierre

}
