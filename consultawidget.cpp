#include "consultawidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSplitter>
#include <QDir>
#include <QMessageBox>
#include <QLabel>

ConsultaWidget::ConsultaWidget(QWidget *parent)
    : QWidget(parent)
{
    stack = new QStackedWidget(this);

    // -----------------------
    //  Página 1: Diseño
    // -----------------------
    paginaDiseno = new QWidget(this);
    QVBoxLayout *layoutDiseno = new QVBoxLayout(paginaDiseno);

    QSplitter *splitterH = new QSplitter(Qt::Horizontal, paginaDiseno);

    scene = new QGraphicsScene(this);
    view = new QGraphicsView(scene, this);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::ScrollHandDrag);

    listaTablas = new QListWidget(this);
    QPushButton *btnAgregar = new QPushButton("Agregar Tabla", this);
    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->addWidget(listaTablas);
    rightLayout->addWidget(btnAgregar);

    QWidget *rightPanel = new QWidget();
    rightPanel->setLayout(rightLayout);
    rightPanel->setMaximumWidth(220);

    splitterH->addWidget(view);
    splitterH->addWidget(rightPanel);
    splitterH->setStretchFactor(0, 3);
    splitterH->setStretchFactor(1, 1);

    gridDesigner = new QueryDesignerWidget(this);

    layoutDiseno->addWidget(splitterH);
    layoutDiseno->addWidget(gridDesigner);

    // -----------------------
    //  Página 2: Resultados
    // -----------------------
    paginaResultado = new QWidget(this);
    QVBoxLayout *layoutRes = new QVBoxLayout(paginaResultado);

    QHBoxLayout *topBar = new QHBoxLayout();
    QLabel *lblTitulo = new QLabel("Resultados de la Consulta");
    btnVolver = new QPushButton("X");
    btnVolver->setFixedSize(25, 25);
    btnVolver->setToolTip("Volver al diseño");

    topBar->addWidget(lblTitulo);
    topBar->addStretch();
    topBar->addWidget(btnVolver);

    connect(btnVolver, &QPushButton::clicked, this, &ConsultaWidget::volverADiseno);

    vistaResultado = new VistaConsulta(this);
    layoutRes->addLayout(topBar);
    layoutRes->addWidget(vistaResultado);

    stack->addWidget(paginaDiseno);
    stack->addWidget(paginaResultado);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(stack);
    setLayout(mainLayout);

    stack->setCurrentWidget(paginaDiseno);

    connect(btnAgregar, &QPushButton::clicked, this, &ConsultaWidget::agregarTabla);
    connect(gridDesigner, &QueryDesignerWidget::ejecutarConsulta,
            this, &ConsultaWidget::ejecutarConsulta);

    QDir dir(QDir::currentPath() + "/tables");
    QStringList archivosMeta = dir.entryList(QStringList() << "*.meta", QDir::Files);
    for (const QString &fileName : archivosMeta) {
        QString nombreTabla = fileName;
        nombreTabla.chop(5);
        listaTablas->addItem(nombreTabla);
    }
}

void ConsultaWidget::agregarTabla()
{
    QListWidgetItem *item = listaTablas->currentItem();
    if (!item) return;

    QString tablaSeleccionada = item->text();
    if (tablas.contains(tablaSeleccionada)) {
        QMessageBox::information(this, "Info", "La tabla ya está en el panel.");
        return;
    }

    QDir dir(QDir::currentPath() + "/tables");
    Metadata meta = Metadata::cargar(dir.filePath(tablaSeleccionada + ".meta"));
    metasDisponibles.append(meta);

    TableItem *tablaItem = new TableItem(meta);
    scene->addItem(tablaItem);
    tablaItem->setPos(rand() % 300, rand() % 300);

    tablas[tablaSeleccionada] = tablaItem;

    connect(tablaItem, &TableItem::iniciarDragCampo,
            this, [=](const QString &tabla, const QString &campo, const QPointF &) {
                gridDesigner->agregarCampo(tabla, campo);
            });
}

void ConsultaWidget::ejecutarConsulta(const QString &sql)
{
    metasDisponibles.clear();

    QDir dir(QDir::currentPath() + "/tables");
    QStringList archivosMeta = dir.entryList(QStringList() << "*.meta", QDir::Files);
    for (const QString &fileName : archivosMeta) {
        Metadata meta = Metadata::cargar(dir.filePath(fileName));
        metasDisponibles.append(meta);
    }

    vistaResultado->mostrarConsulta(sql, metasDisponibles);
    stack->setCurrentWidget(paginaResultado);
}

void ConsultaWidget::volverADiseno()
{
    stack->setCurrentWidget(paginaDiseno);
}

