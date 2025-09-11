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
#include <QScrollBar>
#include <QMimeData>
#include <QDrag>
#include <QApplication>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QPainter>

RelacionesWidget::RelacionesWidget(QWidget *parent)
    : QWidget(parent), arrastrando(false), campoArrastreItem(nullptr)
{
    setStyleSheet("background-color: #f0f0f0; color: #000000;");

    // Inicializar scene y view
    scene = new QGraphicsScene(this);

    view = new QGraphicsView(scene);
    view->setRenderHint(QPainter::Antialiasing);
    view->setDragMode(QGraphicsView::RubberBandDrag);
    view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    view->setAcceptDrops(true);

    // Configurar el área de la escena para que sea lo suficientemente grande
    scene->setSceneRect(-1000, -1000, 2000, 2000);

    crearToolbar();
    crearLayoutPrincipal();
    cargarListaTablas();

    // Conectar eventos del mouse para relaciones
    view->viewport()->installEventFilter(this);
}

RelacionesWidget::~RelacionesWidget()
{
    qDeleteAll(lineasRelaciones.values());
}

void RelacionesWidget::mostrarMensajePersonalizado(const QString &titulo, const QString &mensaje, bool esError)
{
    QMessageBox msgBox;
    msgBox.setWindowTitle(titulo);
    msgBox.setText(mensaje);

    // Estilo personalizado
    if (esError) {
        msgBox.setIcon(QMessageBox::Critical);
        msgBox.setStyleSheet(
            "QMessageBox { background-color: #f8d7da; }"
            "QMessageBox QLabel { color: #721c24; }"
            "QMessageBox QPushButton { background-color: #dc3545; color: white; border: none; padding: 5px 15px; border-radius: 4px; }"
            "QMessageBox QPushButton:hover { background-color: #c82333; }"
            );
    } else {
        msgBox.setIcon(QMessageBox::Information);
        msgBox.setStyleSheet(
            "QMessageBox { background-color: #d4edda; }"
            "QMessageBox QLabel { color: #155724; }"
            "QMessageBox QPushButton { background-color: #28a745; color: white; border: none; padding: 5px 15px; border-radius: 4px; }"
            "QMessageBox QPushButton:hover { background-color: #218838; }"
            );
    }

    msgBox.exec();
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

    QAction *actAutoAjustar = new QAction("Auto Ajustar", this);

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
            mostrarMensajePersonalizado("Advertencia", "Seleccione una tabla primero", true);
            return;
        }

        QString tablaSeleccionada = item->text();
        QDir dir(QDir::currentPath() + "/tables");
        QString metaPath = dir.filePath(tablaSeleccionada + ".meta");

        if (!QFile::exists(metaPath)) {
            mostrarMensajePersonalizado("Error", "Archivo .meta no encontrado: " + metaPath, true);
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
        relacionInfo.clear();
        campoArrastreItem = nullptr;
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
        mostrarMensajePersonalizado("Información", "La tabla ya está en el diagrama");
        return;
    }

    int altura = 30 + (meta.campos.size() * 25);
    int ancho = 200;

    // Subclase interna para restringir movimiento y conectar señales
    class BoundedGroup : public QGraphicsItemGroup {
    public:
        QGraphicsScene *sceneRef;
        RelacionesWidget *parentWidget;
        BoundedGroup(QGraphicsScene *s, RelacionesWidget *parent) : sceneRef(s), parentWidget(parent) {}

    protected:
        QVariant itemChange(GraphicsItemChange change, const QVariant &value) override {
            if (change == ItemPositionChange && sceneRef) {
                QPointF newPos = value.toPointF();
                QRectF bounds = sceneRef->sceneRect();
                QRectF rect = boundingRect();

                // Limitar el movimiento dentro de la escena
                qreal x = qMax(bounds.left(), newPos.x());
                x = qMin(x, bounds.right() - rect.width());

                qreal y = qMax(bounds.top(), newPos.y());
                y = qMin(y, bounds.bottom() - rect.height());

                return QPointF(x, y);
            }
            else if (change == ItemPositionHasChanged) {
                // Actualizar líneas cuando se mueve la tabla
                parentWidget->actualizarLineas();
            }
            return QGraphicsItemGroup::itemChange(change, value);
        }
    };

    BoundedGroup *grupoTabla = new BoundedGroup(scene, this);

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
        campoItem->setData(2, campo.esPK); // Guardar si es PK
        campoItem->setAcceptDrops(true);
        campoItem->setCursor(Qt::PointingHandCursor);

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

    grupoTabla->setFlags(QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges);

    // Posicionar la tabla en un lugar aleatorio dentro de los límites de la vista
    QRectF viewRect = view->viewport()->rect();
    QRectF sceneRect = view->mapToScene(viewRect.toRect()).boundingRect();

    int x = QRandomGenerator::global()->bounded(
        static_cast<int>(sceneRect.left() + 50),
        static_cast<int>(sceneRect.right() - ancho - 50)
        );
    int y = QRandomGenerator::global()->bounded(
        static_cast<int>(sceneRect.top() + 50),
        static_cast<int>(sceneRect.bottom() - altura - 50)
        );

    grupoTabla->setPos(x, y);

    scene->addItem(grupoTabla);

    tablaItems[meta.nombreTabla] = grupoTabla;
    campoItems[meta.nombreTabla] = camposMap;
}

bool RelacionesWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == view->viewport()) {
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            if (mouseEvent->button() == Qt::LeftButton) {
                QPointF scenePos = view->mapToScene(mouseEvent->pos());
                QGraphicsItem *item = scene->itemAt(scenePos, view->transform());

                if (item && item->type() == QGraphicsTextItem::Type) {
                    QGraphicsTextItem *textItem = static_cast<QGraphicsTextItem*>(item);
                    QString tablaNombre = textItem->data(0).toString();
                    QString campoNombre = textItem->data(1).toString();

                    if (!tablaNombre.isEmpty() && !campoNombre.isEmpty()) {
                        // Iniciar arrastre
                        arrastrando = true;
                        tablaArrastre = tablaNombre;
                        campoArrastre = campoNombre;
                        campoArrastreItem = textItem;
                        posicionInicialArrastre = mouseEvent->pos();

                        // Resaltar el campo seleccionado
                        textItem->setDefaultTextColor(QColor(0, 100, 200));

                        // Cambiar cursor para indicar arrastre
                        view->setCursor(Qt::ClosedHandCursor);
                        return true;
                    }
                }
            }
        }
        else if (event->type() == QEvent::MouseMove && arrastrando) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            // Solo procesar si se ha movido lo suficiente para evitar arrastres accidentales
            QPointF delta = mouseEvent->pos() - posicionInicialArrastre;
            if (delta.manhattanLength() > 10) { // Umbral mínimo de movimiento
                view->setCursor(Qt::DragMoveCursor);
            }
            return true;
        }
        else if (event->type() == QEvent::MouseButtonRelease && arrastrando) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);

            // Restaurar cursor
            view->setCursor(Qt::ArrowCursor);

            QPointF scenePos = view->mapToScene(mouseEvent->pos());
            QGraphicsItem *item = scene->itemAt(scenePos, view->transform());

            if (item && item->type() == QGraphicsTextItem::Type) {
                QGraphicsTextItem *textItem = static_cast<QGraphicsTextItem*>(item);
                QString tablaDestino = textItem->data(0).toString();
                QString campoDestino = textItem->data(1).toString();

                if (!tablaDestino.isEmpty() && !campoDestino.isEmpty() &&
                    tablaDestino != tablaArrastre) {
                    // Procesar la relación
                    procesarDragAndDrop(tablaArrastre, campoArrastre, tablaDestino, campoDestino);
                }
            }

            // Restaurar color original
            if (campoArrastreItem) {
                bool esPK = campoArrastreItem->data(2).toBool();
                campoArrastreItem->setDefaultTextColor(esPK ? QColor(200, 0, 0) : Qt::black);
                campoArrastreItem = nullptr;
            }

            arrastrando = false;
            return true;
        }
    }

    return QWidget::eventFilter(obj, event);
}

void RelacionesWidget::procesarDragAndDrop(const QString &tablaOrigen, const QString &campoOrigen,
                                           const QString &tablaDestino, const QString &campoDestino)
{
    // Verificar si ya existe esta relación
    QPair<QString, QString> clave1 = qMakePair(tablaOrigen + "." + campoOrigen, tablaDestino + "." + campoDestino);
    QPair<QString, QString> clave2 = qMakePair(tablaDestino + "." + campoDestino, tablaOrigen + "." + campoOrigen);

    if (lineasRelaciones.contains(clave1) || lineasRelaciones.contains(clave2)) {
        mostrarMensajePersonalizado("Información", "Esta relación ya existe");
        return;
    }

    // Determinar tipo de relación automáticamente
    QString tipoRelacion = determinarTipoRelacion(tablaOrigen, campoOrigen, tablaDestino, campoDestino);

    // Crear diálogo de confirmación personalizado
    QDialog dialog(this);
    dialog.setWindowTitle("Confirmar Relación");
    dialog.setFixedSize(400, 200);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *label = new QLabel(
        QString("¿Crear relación %1 entre:\n\n"
                "• %2.%3\n"
                "• %4.%5?")
            .arg(tipoRelacion)
            .arg(tablaOrigen).arg(campoOrigen)
            .arg(tablaDestino).arg(campoDestino)
        );

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No);

    layout->addWidget(label);
    layout->addWidget(buttonBox);

    // Estilo del diálogo
    dialog.setStyleSheet(
        "QDialog { background-color: #f8f9fa; }"
        "QLabel { color: #212529; font-size: 12px; padding: 10px; }"
        "QDialogButtonBox QPushButton {"
        "   background-color: #007bff; color: white; border: none; padding: 8px 16px; border-radius: 4px;"
        "   min-width: 80px;"
        "}"
        "QDialogButtonBox QPushButton:hover { background-color: #0056b3; }"
        "QDialogButtonBox QPushButton#buttonNo { background-color: #6c757d; }"
        "QDialogButtonBox QPushButton#buttonNo:hover { background-color: #545b62; }"
        );

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        dibujarRelacion(tablaOrigen, campoOrigen, tablaDestino, campoDestino, tipoRelacion);
        emit relacionCreada(tablaOrigen, campoOrigen, tablaDestino, campoDestino, tipoRelacion);
        mostrarMensajePersonalizado("Éxito", "Relación creada correctamente");
    }
}

QString RelacionesWidget::determinarTipoRelacion(const QString &tabla1, const QString &campo1,
                                                 const QString &tabla2, const QString &campo2)
{
    if (!campoItems.contains(tabla1) || !campoItems.contains(tabla2)) {
        return "Uno a Muchos"; // Por defecto
    }

    bool esPK1 = campoItems[tabla1][campo1]->data(2).toBool();
    bool esPK2 = campoItems[tabla2][campo2]->data(2).toBool();

    if (esPK1 && esPK2) {
        return "Uno a Uno";
    } else if (esPK1 || esPK2) {
        return "Uno a Muchos";
    } else {
        return "Muchos a Muchos";
    }
}

void RelacionesWidget::dibujarRelacion(const QString &tabla1, const QString &campo1,
                                       const QString &tabla2, const QString &campo2,
                                       const QString &tipoRelacion)
{
    if (!tablaItems.contains(tabla1) || !tablaItems.contains(tabla2) ||
        !campoItems[tabla1].contains(campo1) || !campoItems[tabla2].contains(campo2)) {
        return;
    }

    QGraphicsTextItem *campoItem1 = campoItems[tabla1][campo1];
    QGraphicsTextItem *campoItem2 = campoItems[tabla2][campo2];

    QPointF p1 = campoItem1->sceneBoundingRect().center();
    QPointF p2 = campoItem2->sceneBoundingRect().center();

    // Crear línea más oscura y lisa
    QGraphicsLineItem *linea = new QGraphicsLineItem(p1.x(), p1.y(), p2.x(), p2.y());

    // Color basado en el tipo de relación
    QColor color;
    if (tipoRelacion == "Uno a Uno") color = QColor(0, 100, 0);      // Verde
    else if (tipoRelacion == "Uno a Muchos") color = QColor(0, 50, 100); // Azul oscuro
    else color = QColor(100, 0, 100);                               // Púrpura

    QPen pen(color);
    pen.setWidth(2);
    pen.setStyle(Qt::SolidLine);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    linea->setPen(pen);

    scene->addItem(linea);

    QPair<QString, QString> clave = qMakePair(tabla1 + "." + campo1, tabla2 + "." + campo2);
    lineasRelaciones[clave] = linea;
    relacionInfo[linea] = qMakePair(tabla1 + "." + campo1, tabla2 + "." + campo2);
}

void RelacionesWidget::actualizarLineas()
{
    for (auto it = lineasRelaciones.begin(); it != lineasRelaciones.end(); ++it) {
        QGraphicsLineItem *linea = it.value();
        QStringList partesOrigen = it.key().first.split(".");
        QStringList partesDestino = it.key().second.split(".");

        if (partesOrigen.size() == 2 && partesDestino.size() == 2) {
            QString tabla1 = partesOrigen[0];
            QString campo1 = partesOrigen[1];
            QString tabla2 = partesDestino[0];
            QString campo2 = partesDestino[1];

            if (tablaItems.contains(tabla1) && tablaItems.contains(tabla2) &&
                campoItems[tabla1].contains(campo1) && campoItems[tabla2].contains(campo2)) {

                QPointF p1 = campoItems[tabla1][campo1]->sceneBoundingRect().center();
                QPointF p2 = campoItems[tabla2][campo2]->sceneBoundingRect().center();

                linea->setLine(p1.x(), p1.y(), p2.x(), p2.y());
            }
        }
    }
}

void RelacionesWidget::closeEvent(QCloseEvent *event)
{
    emit cerrada();
    event->accept();
}

void RelacionesWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // No es necesario ajustar la escena al viewport para permitir scroll
}
