#include "relacioneswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QFrame>
#include <QScrollArea>
#include <QLabel>
#include <QDir>
#include <QPushButton>
#include <QGroupBox>
#include <QCloseEvent>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QMouseEvent>
#include <QMessageBox>
#include <QInputDialog>
#include <QRandomGenerator>

RelacionesWidget::RelacionesWidget(QWidget *parent)
    : QWidget(parent), seleccionandoOrigen(false)
{
    setStyleSheet("background-color: #f0f0f0; color: #000000;");

    // Inicializar scene y view
    scene = new QGraphicsScene(this);

    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

    // Limitar el área de movimiento de las cards al panel visible
    scene->setSceneRect(view->viewport()->rect());

    crearToolbar();
    crearLayoutPrincipal();
    cargarListaTablas();

    // Conectar eventos del mouse para relaciones
    view->viewport()->installEventFilter(this);
}

RelacionesWidget::~RelacionesWidget()
{
    qDeleteAll(lineasRelaciones);
}

void RelacionesWidget::crearSistemaRelaciones()
{
    // Ya no se necesita crear la scene aquí, se inicializa en el constructor
}

void RelacionesWidget::crearToolbar()
{
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    // Botón Cerrar
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

    // Panel izquierdo: lista de tablas
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
    panelTablas->setFixedWidth(250);

    // Área central: vista de relaciones
    QVBoxLayout *viewLayout = new QVBoxLayout();

    // Toolbar de relaciones - arriba del view
    QToolBar *relToolbar = new QToolBar();
    relToolbar->setIconSize(QSize(16, 16));

    QAction *actCrearRel = new QAction("Crear Relación", this);
    QAction *actEliminarRel = new QAction("Eliminar Relación", this);
    QAction *actAutoAjustar = new QAction("Auto Ajustar", this);

    relToolbar->addAction(actCrearRel);
    relToolbar->addAction(actEliminarRel);
    relToolbar->addAction(actAutoAjustar);

    crearSistemaRelaciones();

    viewLayout->addWidget(relToolbar);
    viewLayout->addWidget(view, 1);

    contentLayout->addWidget(panelTablas);
    contentLayout->addLayout(viewLayout, 1);

    mainLayout->addLayout(contentLayout);

    // Conectar señales
    connect(btnAgregar, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = listaTablas->currentItem();
        if (!item) {
            QMessageBox::warning(this, "Advertencia", "Seleccione una tabla primero");
            return;
        }

        QString tablaSeleccionada = item->text();
        QDir dir(QDir::currentPath() + "/tables");
        QString metaPath = dir.filePath(tablaSeleccionada + ".meta");

        if (!QFile::exists(metaPath)) {
            QMessageBox::warning(this, "Error", "Archivo .meta no encontrado: " + metaPath);
            return;
        }

        Metadata meta = Metadata::cargar(metaPath);
        agregarTablaAScene(meta);
    });

    connect(btnLimpiar, &QPushButton::clicked, this, [this]() {
        scene->clear();
        tablaItems.clear();
        campoItems.clear();
        lineasRelaciones.clear();
    });

    connect(actCrearRel, &QAction::triggered, this, [this]() {
        seleccionandoOrigen = true;
        tablaOrigen.clear();
        campoOrigen.clear();
        QMessageBox::information(this, "Crear Relación",
                                 "Haga click en el campo de origen y luego en el campo de destino");
    });

    connect(actAutoAjustar, &QAction::triggered, this, [this]() {
        view->fitInView(scene->itemsBoundingRect(), Qt::KeepAspectRatio);
    });
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
        if (!nombreTabla.isEmpty()) listaTablas->addItem(nombreTabla);
    }
}

void RelacionesWidget::agregarTablaAScene(const Metadata &meta)
{
    if (tablaItems.contains(meta.nombreTabla)) {
        QMessageBox::information(this, "Información", "La tabla ya está en el diagrama");
        return;
    }

    int altura = 30 + (meta.campos.size() * 25);
    int ancho = 200;

    // Subclase interna para restringir movimiento
    class BoundedGroup : public QGraphicsItemGroup {
    public:
        QGraphicsScene *sceneRef;
        BoundedGroup(QGraphicsScene *s) : sceneRef(s) {}

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
            if (change == ItemPositionChange && sceneRef) {
                QPointF newPos = value.toPointF();
                QRectF bounds = sceneRef->sceneRect();
                QRectF rect = boundingRect();

                qreal x = qBound(bounds.left(), newPos.x(),
                                 bounds.right() - rect.width());
                qreal y = qBound(bounds.top(), newPos.y(),
                                 bounds.bottom() - rect.height());
                return QPointF(x, y);
            }
            return QGraphicsItemGroup::itemChange(change, value);
        }
    };

    BoundedGroup *grupoTabla = new BoundedGroup(scene);

    QGraphicsRectItem *tablaRect = new QGraphicsRectItem(0, 0, ancho, altura);
    tablaRect->setBrush(QBrush(Qt::white));
    tablaRect->setPen(QPen(QColor(43, 87, 154), 2));

    QGraphicsRectItem *tituloRect = new QGraphicsRectItem(0, 0, ancho, 25);
    tituloRect->setBrush(QBrush(QColor(43, 87, 154)));
    tituloRect->setPen(Qt::NoPen);

    QGraphicsTextItem *titulo = new QGraphicsTextItem(meta.nombreTabla);
    titulo->setDefaultTextColor(Qt::white);
    titulo->setPos(5, 5);
    QFont font = titulo->font();
    font.setBold(true);
    font.setPointSize(9);
    titulo->setFont(font);

    grupoTabla->addToGroup(tablaRect);
    grupoTabla->addToGroup(tituloRect);
    grupoTabla->addToGroup(titulo);

    QMap<QString, QGraphicsTextItem*> camposMap;
    for (int i = 0; i < meta.campos.size(); ++i) {
        const Campo &campo = meta.campos[i];
        QString textoCampo = campo.nombre + " (" + campo.tipo + ")";
        if (campo.esPK) textoCampo = "🔑 " + textoCampo;

        QGraphicsTextItem *campoItem = new QGraphicsTextItem(textoCampo);
        campoItem->setPos(5, 30 + i * 25);
        campoItem->setData(0, meta.nombreTabla);
        campoItem->setData(1, campo.nombre);

        if (campo.esPK) {
            QFont campFont = campoItem->font();
            campFont.setBold(true);
            campoItem->setFont(campFont);
            campoItem->setDefaultTextColor(QColor(200, 0, 0));
        } else {
            campoItem->setDefaultTextColor(Qt::black);
        }

        grupoTabla->addToGroup(campoItem);
        camposMap[campo.nombre] = campoItem;
    }

    grupoTabla->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable);

    int x = QRandomGenerator::global()->bounded(100, 400);
    int y = QRandomGenerator::global()->bounded(100, 300);
    grupoTabla->setPos(x, y);

    scene->addItem(grupoTabla);

    tablaItems[meta.nombreTabla] = grupoTabla;
    campoItems[meta.nombreTabla] = camposMap;
}

bool RelacionesWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == view->viewport() && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

        if (mouseEvent->button() == Qt::LeftButton && seleccionandoOrigen) {
            QPointF scenePos = view->mapToScene(mouseEvent->pos());
            QGraphicsItem *item = scene->itemAt(scenePos, view->transform());

            if (item && item->type() == QGraphicsTextItem::Type) {
                QGraphicsTextItem *textItem = static_cast<QGraphicsTextItem*>(item);
                QString tablaNombre = textItem->data(0).toString();
                QString campoNombre = textItem->data(1).toString();

                if (!tablaNombre.isEmpty() && !campoNombre.isEmpty()) {
                    procesarClickTabla(tablaNombre, campoNombre);
                    return true;
                }
            }
        }
    }

    return QWidget::eventFilter(obj, event);
}

void RelacionesWidget::procesarClickTabla(const QString &tablaNombre, const QString &campoNombre)
{
    if (seleccionandoOrigen) {
        if (tablaOrigen.isEmpty()) {
            tablaOrigen = tablaNombre;
            campoOrigen = campoNombre;
            QMessageBox::information(this, "Relación",
                                     QString("Origen seleccionado: %1.%2\nAhora seleccione el campo destino")
                                         .arg(tablaNombre).arg(campoNombre));
        } else {
            if (tablaOrigen == tablaNombre) {
                QMessageBox::warning(this, "Error", "No puede relacionar una tabla consigo misma");
            } else {
                QStringList tipos;
                tipos << "Uno a Uno" << "Uno a Muchos" << "Muchos a Muchos";

                QString tipoRelacion = QInputDialog::getItem(this, "Tipo de Relación",
                                                             "Seleccione el tipo de relación:", tipos, 1, false);

                if (!tipoRelacion.isEmpty()) {
                    dibujarRelacion(tablaOrigen, campoOrigen, tablaNombre, campoNombre);
                    emit relacionCreada(tablaOrigen, campoOrigen, tablaNombre, campoNombre);
                }
            }
            tablaOrigen.clear();
            campoOrigen.clear();
            seleccionandoOrigen = false;
        }
    }
}

void RelacionesWidget::dibujarRelacion(const QString &tabla1, const QString &campo1,
                                       const QString &tabla2, const QString &campo2)
{
    if (!tablaItems.contains(tabla1) || !tablaItems.contains(tabla2) ||
        !campoItems[tabla1].contains(campo1) || !campoItems[tabla2].contains(campo2)) {
        return;
    }

    QGraphicsTextItem *campoItem1 = campoItems[tabla1][campo1];
    QGraphicsTextItem *campoItem2 = campoItems[tabla2][campo2];

    QPointF p1 = campoItem1->sceneBoundingRect().center();
    QPointF p2 = campoItem2->sceneBoundingRect().center();

    QGraphicsLineItem *linea = new QGraphicsLineItem(p1.x(), p1.y(), p2.x(), p2.y());
    linea->setPen(QPen(QColor(0, 100, 200), 2, Qt::DashLine));

    scene->addItem(linea);
    lineasRelaciones.append(linea);
}

void RelacionesWidget::closeEvent(QCloseEvent *event)
{
    emit cerrada();
    event->accept();
}

void RelacionesWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (view && scene) {
        scene->setSceneRect(view->viewport()->rect());
    }
}

