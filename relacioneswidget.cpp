#include "relacioneswidget.h"
#include "relaciondialog.h"
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
#include <QMouseEvent>
#include <QDebug>
#include <QFile>
#include <QTextStream>

RelacionesWidget::RelacionesWidget(QWidget *parent)
    : QWidget(parent)
{
    scene = new QGraphicsScene(this);
    scene->setSceneRect(-1000, -1000, 2000, 2000);

    view = new RelacionesView(this);
    view->setScene(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // Eventos de mouse desde la vista
    connect(view, &RelacionesView::mouseMovedEnScene, this, [this](const QPointF &pos) {
        if (arrastrando && lineaTemporal) {
            lineaTemporal->setLine(QLineF(puntoDrag, pos));
        }
    });

    connect(view, &RelacionesView::mouseReleasedEnScene, this, [this](const QPointF &pos) {
        if (!arrastrando) return;

        QString tablaDestino, campoDestino;
        for (auto *tabla : tablas.values()) {
            for (const CampoVisual &cv : tabla->getCamposVisuales()) {
                QRectF rectScene = tabla->mapToScene(cv.rect).boundingRect();
                if (rectScene.contains(pos)) {
                    tablaDestino = tabla->getTableName();
                    campoDestino = cv.nombre;
                    break;
                }
            }
            if (!tablaDestino.isEmpty()) break;
        }

        scene->removeItem(lineaTemporal);
        delete lineaTemporal;
        lineaTemporal = nullptr;
        arrastrando = false;

        if (tablaDestino.isEmpty() || tablaDestino == tablaDrag) return;

        // 🔹 Determinar tipo automáticamente
        bool origenEsPK = false, destinoEsPK = false;
        for (const Campo &c : tablas[tablaDrag]->getMetadata().campos)
            if (c.nombre == campoDrag) { origenEsPK = c.esPK; break; }
        for (const Campo &c : tablas[tablaDestino]->getMetadata().campos)
            if (c.nombre == campoDestino) { destinoEsPK = c.esPK; break; }

        TipoRelacion tipo;
        QString tipoTexto;
        if (origenEsPK && destinoEsPK) {
            tipo = TipoRelacion::UnoAUno;
            tipoTexto = "Uno a Uno";
        } else if (origenEsPK || destinoEsPK) {
            tipo = TipoRelacion::UnoAMuchos;
            tipoTexto = "Uno a Varios";
        } else {
            tipo = TipoRelacion::MuchosAMuchos;
            tipoTexto = "Varios a Varios";
        }

        // 🔹 Mostrar diálogo estilo Access
        RelacionDialog dlg(tablaDrag, campoDrag, tablaDestino, campoDestino, tipoTexto, this);
        if (dlg.exec() != QDialog::Accepted) return;

        // 🔹 Crear relación en escena
        RelationItem *rel = new RelationItem(tablas[tablaDrag], campoDrag,
                                             tablas[tablaDestino], campoDestino,
                                             tipo);
        scene->addItem(rel);
        relaciones.append(rel);

        emit relacionCreada(tablaDrag, campoDrag, tablaDestino, campoDestino);
    });

    crearToolbar();
    crearLayoutPrincipal();
    cargarListaTablas();
    cargarRelacionesPrevias(); // 🔹 cargar relaciones al abrir
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

    connect(tablaItem, &TableItem::iniciarDragCampo,
            this, [this](const QString &tabla, const QString &campo, const QPointF &pos) {
                tablaDrag = tabla;
                campoDrag = campo;
                puntoDrag = pos;
                arrastrando = true;

                lineaTemporal = scene->addLine(QLineF(pos, pos), QPen(Qt::gray, 1, Qt::DashLine));
            });
}

void RelacionesWidget::limpiarTodo()
{
    scene->clear();
    tablas.clear();
    relaciones.clear();
}

void RelacionesWidget::cargarRelacionesPrevias()
{
    QFile relacionesFile("relationships.dat");
    if (!relacionesFile.open(QIODevice::ReadOnly)) return;

    QTextStream in(&relacionesFile);
    QList<QStringList> relacionesPendientes;

    while (!in.atEnd()) {
        QString line = in.readLine();
        QStringList parts = line.split("|");
        if (parts.size() != 4) continue;
        relacionesPendientes.append(parts);
    }
    relacionesFile.close();

    // 🔹 Asegurar que todas las tablas existan en la escena
    QDir dir(QDir::currentPath() + "/tables");
    for (const QStringList &rel : relacionesPendientes) {
        QString t1 = rel[0];
        QString t2 = rel[2];

        for (const QString &t : {t1, t2}) {
            if (!tablas.contains(t)) {
                Metadata meta = Metadata::cargar(dir.filePath(t + ".meta"));
                TableItem *tablaItem = new TableItem(meta);
                scene->addItem(tablaItem);

                int x = QRandomGenerator::global()->bounded(-200, 200);
                int y = QRandomGenerator::global()->bounded(-200, 200);
                tablaItem->setPos(x, y);

                tablas[t] = tablaItem;

                connect(tablaItem, &TableItem::iniciarDragCampo,
                        this, [this](const QString &tabla, const QString &campo, const QPointF &pos) {
                            tablaDrag = tabla;
                            campoDrag = campo;
                            puntoDrag = pos;
                            arrastrando = true;
                            lineaTemporal = scene->addLine(QLineF(pos, pos), QPen(Qt::gray, 1, Qt::DashLine));
                        });
            }
        }
    }

    // 🔹 Crear relaciones visuales
    for (const QStringList &rel : relacionesPendientes) {
        QString t1 = rel[0];
        QString c1 = rel[1];
        QString t2 = rel[2];
        QString c2 = rel[3];

        if (tablas.contains(t1) && tablas.contains(t2)) {
            bool origenEsPK = false, destinoEsPK = false;
            for (const Campo &c : tablas[t1]->getMetadata().campos)
                if (c.nombre == c1) { origenEsPK = c.esPK; break; }
            for (const Campo &c : tablas[t2]->getMetadata().campos)
                if (c.nombre == c2) { destinoEsPK = c.esPK; break; }

            TipoRelacion tipo;
            if (origenEsPK && destinoEsPK)
                tipo = TipoRelacion::UnoAUno;
            else if (origenEsPK || destinoEsPK)
                tipo = TipoRelacion::UnoAMuchos;
            else
                tipo = TipoRelacion::MuchosAMuchos;

            RelationItem *relItem = new RelationItem(tablas[t1], c1,
                                                     tablas[t2], c2, tipo);
            scene->addItem(relItem);
            relaciones.append(relItem);

            qDebug() << "[DEBUG] Relación cargada:" << t1 << "." << c1
                     << "->" << t2 << "." << c2;
        }
    }
}


void RelacionesWidget::closeEvent(QCloseEvent *event)
{
    emit cerrada();
    event->accept();
}
