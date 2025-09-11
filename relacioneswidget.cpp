#include "relacioneswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QFrame>
#include <QGroupBox>
#include <QPushButton>
#include <QDir>
#include <QCloseEvent>
#include <QMessageBox>
#include <QRandomGenerator>
#include <QInputDialog>

RelacionesWidget::RelacionesWidget(QWidget *parent)
    : QWidget(parent)
{
    scene = new QGraphicsScene(this);
    scene->setSceneRect(-1000, -1000, 2000, 2000);

    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    crearToolbar();
    crearLayoutPrincipal();
    cargarListaTablas();
}

RelacionesWidget::~RelacionesWidget()
{
    qDeleteAll(relaciones);
    relaciones.clear();
    tablas.clear();
}

void RelacionesWidget::crearToolbar()
{
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    QToolButton *btnCerrar = new QToolButton();
    btnCerrar->setText("Cerrar");
    connect(btnCerrar, &QToolButton::clicked, this, &QWidget::close);

    toolbar->addWidget(btnCerrar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(toolbar);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
}

void RelacionesWidget::crearLayoutPrincipal()
{
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());
    if (!mainLayout) {
        mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(0);
        mainLayout->setContentsMargins(0, 0, 0, 0);
    }

    QHBoxLayout *contentLayout = new QHBoxLayout();

    // Panel izquierdo
    QFrame *panelTablas = new QFrame();
    QVBoxLayout *panelLayout = new QVBoxLayout(panelTablas);

    QGroupBox *grupoTablas = new QGroupBox("Tablas Disponibles");
    QVBoxLayout *tablasLayout = new QVBoxLayout(grupoTablas);

    listaTablas = new QListWidget();
    tablasLayout->addWidget(listaTablas);

    QPushButton *btnAgregar = new QPushButton("Agregar Tabla");
    QPushButton *btnLimpiar = new QPushButton("Limpiar Todo");
    tablasLayout->addWidget(btnAgregar);
    tablasLayout->addWidget(btnLimpiar);

    panelLayout->addWidget(grupoTablas);
    panelTablas->setFixedWidth(200);

    QVBoxLayout *viewLayout = new QVBoxLayout();
    viewLayout->addWidget(view, 1);

    contentLayout->addWidget(panelTablas);
    contentLayout->addLayout(viewLayout, 1);

    mainLayout->addLayout(contentLayout);

    connect(btnAgregar, &QPushButton::clicked, this, &RelacionesWidget::agregarTabla);
    connect(btnLimpiar, &QPushButton::clicked, this, &RelacionesWidget::limpiarTodo);
}

void RelacionesWidget::cargarListaTablas()
{
    if (!listaTablas) return;
    listaTablas->clear();

    QDir dir(QDir::currentPath() + "/tables");
    if (!dir.exists()) {
        listaTablas->addItem("No se encontró el directorio 'tables'");
        return;
    }

    QStringList archivosMeta = dir.entryList(QStringList() << "*.meta", QDir::Files);
    if (archivosMeta.isEmpty()) {
        listaTablas->addItem("No hay tablas disponibles");
        return;
    }

    for (const QString &fileName : archivosMeta) {
        QString nombreTabla = fileName;
        nombreTabla.chop(5);
        if (nombreTabla.compare("Relaciones", Qt::CaseInsensitive) == 0)
            continue;
        listaTablas->addItem(nombreTabla);
    }
}

void RelacionesWidget::agregarTabla()
{
    QListWidgetItem *item = listaTablas->currentItem();
    if (!item) {
        QMessageBox::warning(this, "Advertencia", "Seleccione una tabla primero");
        return;
    }

    QString tablaSeleccionada = item->text();
    if (tablas.contains(tablaSeleccionada)) {
        QMessageBox::information(this, "Info", "La tabla ya está en el diagrama");
        return;
    }

    QDir dir(QDir::currentPath() + "/tables");
    Metadata meta = Metadata::cargar(dir.filePath(tablaSeleccionada + ".meta"));

    TableItem *tablaItem = new TableItem(meta);
    scene->addItem(tablaItem);

    int x = QRandomGenerator::global()->bounded(-200, 200);
    int y = QRandomGenerator::global()->bounded(-200, 200);
    tablaItem->setPos(x, y);

    tablas[tablaSeleccionada] = tablaItem;

    connect(tablaItem, &TableItem::campoSeleccionado,
            this, [this](const QString &tabla, const QString &campo, const QPointF &) {
                if (!esperandoDestino) {
                    tablaOrigen = tabla;
                    campoOrigen = campo;
                    esperandoDestino = true;
                } else {
                    QString tablaDestino = tabla;
                    QString campoDestino = campo;

                    // 🔹 Diálogo de confirmación
                    QStringList tipos = {"Uno a Uno", "Uno a Muchos", "Muchos a Muchos"};
                    bool ok;
                    QString tipoSeleccionado = QInputDialog::getItem(this, "Tipo de Relación",
                                                                     "Seleccione el tipo de relación:",
                                                                     tipos, 1, false, &ok);
                    if (!ok) {
                        esperandoDestino = false;
                        return;
                    }

                    TipoRelacion tipo;
                    if (tipoSeleccionado == "Uno a Uno")
                        tipo = TipoRelacion::UnoAUno;
                    else if (tipoSeleccionado == "Uno a Muchos")
                        tipo = TipoRelacion::UnoAMuchos;
                    else
                        tipo = TipoRelacion::MuchosAMuchos;

                    RelationItem *rel = new RelationItem(tablas[tablaOrigen], campoOrigen,
                                                         tablas[tablaDestino], campoDestino,
                                                         tipo);
                    scene->addItem(rel);
                    relaciones.append(rel);

                    emit relacionCreada(tablaOrigen, campoOrigen, tablaDestino, campoDestino);

                    esperandoDestino = false;
                }
            });
}

void RelacionesWidget::limpiarTodo()
{
    scene->clear();
    tablas.clear();
    relaciones.clear();
}

void RelacionesWidget::closeEvent(QCloseEvent *event)
{
    emit cerrada();
    event->accept();
}
