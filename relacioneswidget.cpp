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

        // 🔹 Determinar tipo automáticamente (1:1, 1:N, N:M)
        bool origenEsPK = false, destinoEsPK = false;
        for (const Campo &c : tablas[tablaDrag]->getMetadata().campos)
            if (c.nombre == campoDrag) { origenEsPK = c.esPK; break; }
        for (const Campo &c : tablas[tablaDestino]->getMetadata().campos)
            if (c.nombre == campoDestino) { destinoEsPK = c.esPK; break; }

        TipoRelacion tipo;
        QString tipoTexto;
        if (origenEsPK && destinoEsPK) {
            if (campoDrag == campoDestino) {
                tipo = TipoRelacion::UnoAUno;
                tipoTexto = "Uno a Uno";
            } else {
                QMessageBox::warning(this,
                                     "Relación inválida",
                                     "No se puede crear una relación 1 a 1 entre dos llaves primarias con nombres distintos.");
                return; // ❌ cancelar relación
            }
        } else if (origenEsPK || destinoEsPK) {
            tipo = TipoRelacion::UnoAMuchos;
            tipoTexto = "Uno a Varios";
        } else {
            tipo = TipoRelacion::MuchosAMuchos;
            tipoTexto = "Varios a Varios";
        }

        // 🔹 Validar compatibilidad de tipos ANTES del dialog
        const Metadata &metaOrigen = tablas[tablaDrag]->getMetadata();
        const Metadata &metaDestino = tablas[tablaDestino]->getMetadata();

        Campo campoO, campoD;
        bool encontradoO = false, encontradoD = false;

        for (const Campo &c : metaOrigen.campos) {
            if (c.nombre == campoDrag) { campoO = c; encontradoO = true; break; }
        }
        for (const Campo &c : metaDestino.campos) {
            if (c.nombre == campoDestino) { campoD = c; encontradoD = true; break; }
        }

        if (encontradoO && encontradoD) {
            if (!validarCompatibilidadTipos(campoO, campoD)) {
                return; // ❌ Cancelar relación sin mostrar RelacionDialog
            }
        }

        if ((campoO.esPK && campoD.esPK) && (campoO.nombre != campoD.nombre)) {
            QMessageBox::warning(this,
                                 "Relación inválida",
                                 QString("No se puede relacionar el campo '%1' con '%2' porque tienen nombres diferentes.\n"
                                         "Los campos relacionados deben tener el mismo nombre al ser llaves primarias.")
                                     .arg(campoO.nombre)
                                     .arg(campoD.nombre));
            return; // ❌ Cancelar relación
        }

        // 🔹 Validar que los nombres de campo coincidan (case-insensitive)
        if (campoDrag.compare(campoDestino, Qt::CaseInsensitive) != 0) {
            QMessageBox::warning(this, "Relación inválida",
                                 QString("No se puede relacionar '%1' con '%2' porque los nombres son diferentes.\n"
                                         "Solo se permiten relaciones entre campos con el mismo nombre.")
                                     .arg(campoDrag, campoDestino));
            return; // ❌ cancelar sin abrir RelacionDialog
        }

        RelacionDialog dlg(tablaDrag, campoDrag, tablaDestino, campoDestino,
                           origenEsPK, destinoEsPK, this);

        // 🔹 Preseleccionar tipo correcto
        if (tipo == TipoRelacion::UnoAUno) {
            dlg.setTipoRelacion("1:1");
        } else if (tipo == TipoRelacion::UnoAMuchos) {
            dlg.setTipoRelacion("1:M");
        } else if (tipo == TipoRelacion::MuchosAMuchos) {
            dlg.setTipoRelacion("M:M");
        }

        if (dlg.exec() != QDialog::Accepted) return;



        QString tipoRelacion = dlg.getTipoRelacion();

        // 🔹 Crear relación en escena
        RelationItem *rel = nullptr;

        if (tipo == TipoRelacion::UnoAMuchos) {
            if (origenEsPK) {
                // origen es PK → origen va como "1"
                rel = new RelationItem(tablas[tablaDrag], campoDrag,
                                       tablas[tablaDestino], campoDestino,
                                       tipo);
            } else {
                // destino es PK → destino va como "1"
                rel = new RelationItem(tablas[tablaDestino], campoDestino,
                                       tablas[tablaDrag], campoDrag,
                                       tipo);
            }
        } else {
            // Para 1:1 y M:M no importa el orden
            rel = new RelationItem(tablas[tablaDrag], campoDrag,
                                   tablas[tablaDestino], campoDestino,
                                   tipo);
        }

        scene->addItem(rel);
        relaciones.append(rel);


        emit relacionCreada(tablaDrag, campoDrag, tablaDestino, campoDestino, tipoRelacion);
        guardarRelacionEnArchivo(tablaDrag, campoDrag, tablaDestino, campoDestino);
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

void RelacionesWidget::guardarRelacionEnArchivo(const QString &tabla1, const QString &campo1,
                                                const QString &tabla2, const QString &campo2)
{
    QFile relacionesFile("relationships.dat");
    if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        QTextStream out(&relacionesFile);
        out << tabla1 << "|" << campo1 << "|" << tabla2 << "|" << campo2 << "\n";
        relacionesFile.close();

        // Emitir señal para que se actualicen los validadores
        emit relacionesActualizadas();
    }
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
    QPushButton *btnEliminarRelacion = new QPushButton("Eliminar Relación");
    tablasLayout->addWidget(btnAgregar);
    tablasLayout->addWidget(btnLimpiar);
    tablasLayout->addWidget(btnEliminarRelacion);

    panelLayout->addWidget(grupoTablas);
    panelTablas->setFixedWidth(200);

    QVBoxLayout *viewLayout = new QVBoxLayout();
    viewLayout->addWidget(view, 1);

    contentLayout->addWidget(panelTablas);
    contentLayout->addLayout(viewLayout, 1);

    mainLayout->addLayout(contentLayout);

    connect(btnAgregar, &QPushButton::clicked, this, &RelacionesWidget::agregarTabla);
    connect(btnLimpiar, &QPushButton::clicked, this, &RelacionesWidget::limpiarTodo);
    connect(btnEliminarRelacion, &QPushButton::clicked, this, &RelacionesWidget::eliminarRelacionSeleccionada);
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
    tablaItem->update();
    scene->update();

    int index = tablas.size();
    int gridX = (index % 5) * 200;   // máximo 5 por fila
    int gridY = (index / 5) * 200;
    tablaItem->setPos(gridX, gridY);


    tablas[tablaSeleccionada] = tablaItem;

    connect(tablaItem, &TableItem::iniciarDragCampo,
            this, [this](const QString &tabla, const QString &campo, const QPointF &pos) {
                tablaDrag = tabla;
                campoDrag = campo;
                puntoDrag = pos;
                arrastrando = true;

                lineaTemporal = scene->addLine(QLineF(pos, pos), QPen(Qt::gray, 1, Qt::DashLine));
            });

    for (RelationItem *rel : relaciones) {
        rel->updatePosition();
    }

}

void RelacionesWidget::limpiarTodo()
{
    // Limpiar la escena y estructuras en memoria
    scene->clear();
    tablas.clear();
    relaciones.clear();

    // Vaciar el archivo de relaciones
    QFile relacionesFile("relationships.dat");
    if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        relacionesFile.close();
    }
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

    // 🔹 Asegurar que todas las tablas existan en la escena con metadata actualizado
    QDir dir(QDir::currentPath() + "/tables");
    for (const QStringList &rel : relacionesPendientes) {
        QString t1 = rel[0];
        QString t2 = rel[2];

        for (const QString &t : {t1, t2}) {
            // ✅ Eliminar y recargar siempre desde el .meta
            if (tablas.contains(t)) {
                TableItem *old = tablas[t];
                QPointF pos = old->pos();
                scene->removeItem(old);
                delete old;
                tablas.remove(t);

                Metadata meta = Metadata::cargar(dir.filePath(t + ".meta"));
                TableItem *tablaItem = new TableItem(meta);
                scene->addItem(tablaItem);
                tablaItem->setPos(pos);
                tablaItem->update();
                scene->update();
                tablas[t] = tablaItem;

                connect(tablaItem, &TableItem::iniciarDragCampo,
                        this, [this](const QString &tabla, const QString &campo, const QPointF &pos) {
                            tablaDrag = tabla;
                            campoDrag = campo;
                            puntoDrag = pos;
                            arrastrando = true;
                            lineaTemporal = scene->addLine(QLineF(pos, pos), QPen(Qt::gray, 1, Qt::DashLine));
                        });
            } else {
                // Caso normal: tabla no estaba en escena
                Metadata meta = Metadata::cargar(dir.filePath(t + ".meta"));
                TableItem *tablaItem = new TableItem(meta);
                scene->addItem(tablaItem);

                int index = tablas.size();
                int gridX = (index % 5) * 50;
                int gridY = (index / 5) * 50;
                tablaItem->setPos(gridX, gridY);
                tablaItem->update();
                scene->update();
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

    // 🔹 Crear relaciones visuales (igual que antes)
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
            relItem->updatePosition();
            relaciones.append(relItem);
        }
    }

     validarRelacionesExistentes();

    for (RelationItem *rel : relaciones) {
        rel->updatePosition();
    }
}



void RelacionesWidget::closeEvent(QCloseEvent *event)
{
    emit cerrada();
    event->accept();
}

bool RelacionesWidget::validarCompatibilidadTipos(const Campo &campoOrigen, const Campo &campoDestino) {
    if (campoOrigen.tipo == campoDestino.tipo) {
        return true;
    }

    QMessageBox::warning(this,
                         "Relación inválida",
                         QString("No se puede relacionar el campo '%1' (%2) con '%3' (%4).")
                             .arg(campoOrigen.nombre).arg(campoOrigen.tipo)
                             .arg(campoDestino.nombre).arg(campoDestino.tipo));
    return false;
}
void RelacionesWidget::eliminarRelacionSeleccionada()
{
    RelationItem *relSeleccionada = nullptr;

    // Buscar la relación seleccionada en la lista
    for (RelationItem *rel : relaciones) {
        if (rel->isSelected()) {
            relSeleccionada = rel;
            break;
        }
    }

    if (!relSeleccionada) {
        QMessageBox::warning(this, "Eliminar Relación", "No hay ninguna relación seleccionada.");
        return;
    }

    // Eliminar de la escena
    scene->removeItem(relSeleccionada);

    // Eliminar del vector
    relaciones.removeOne(relSeleccionada);

    // Eliminar del archivo relationships.dat
    QFile relacionesFile("relationships.dat");
    if (relacionesFile.open(QIODevice::ReadOnly)) {
        QStringList lineasValidas;
        QTextStream in(&relacionesFile);
        while (!in.atEnd()) {
            QString linea = in.readLine();
            if (!linea.contains(relSeleccionada->getSource()->getTableName() + "|" + relSeleccionada->getCampoSource() + "|" +
                                relSeleccionada->getDest()->getTableName() + "|" + relSeleccionada->getCampoDest())) {
                lineasValidas << linea;
            }
        }
        relacionesFile.close();

        if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&relacionesFile);
            for (const QString &linea : lineasValidas)
                out << linea << "\n";
            relacionesFile.close();
        }
    }

    delete relSeleccionada; // liberar memoria
}
void RelacionesWidget::refrescarTablas()
{
    QDir dir(QDir::currentPath() + "/tables");

    for (auto it = tablas.begin(); it != tablas.end(); ++it) {
        QString nombreTabla = it.key();
        TableItem *item = it.value();

        Metadata meta = Metadata::cargar(dir.filePath(nombreTabla + ".meta"));
        item->setMetadata(meta);   // 🔹 recarga en vivo, sin borrar
        item->update();
    }

    // 🔹 Validar relaciones existentes después de actualizar metadatos
    validarRelacionesExistentes();

    for (RelationItem *rel : relaciones) {
        rel->updatePosition();
    }
}

void RelacionesWidget::validarRelacionesExistentes()
{
    QList<RelationItem*> relacionesInvalidas;

    for (RelationItem *rel : relaciones) {
        QString tablaOrigen = rel->getSource()->getTableName();
        QString tablaDestino = rel->getDest()->getTableName();
        QString campoOrigen = rel->getCampoSource();
        QString campoDestino = rel->getCampoDest();

        // Verificar si las tablas aún existen
        if (!tablas.contains(tablaOrigen) || !tablas.contains(tablaDestino)) {
            relacionesInvalidas.append(rel);
            continue;
        }

        // Obtener metadatos actualizados
        const Metadata &metaOrigen = tablas[tablaOrigen]->getMetadata();
        const Metadata &metaDestino = tablas[tablaDestino]->getMetadata();

        // Buscar campos en los metadatos actuales
        Campo campoO, campoD;
        bool encontradoO = false, encontradoD = false;

        for (const Campo &c : metaOrigen.campos) {
            if (c.nombre == campoOrigen) { campoO = c; encontradoO = true; break; }
        }
        for (const Campo &c : metaDestino.campos) {
            if (c.nombre == campoDestino) { campoD = c; encontradoD = true; break; }
        }

        // Validar si la relación sigue siendo válida
        bool origenEsPK = campoO.esPK;
        bool destinoEsPK = campoD.esPK;
        if (!encontradoO || !encontradoD) {
            relacionesInvalidas.append(rel);
        }
        else if (campoO.tipo != campoD.tipo) {
            relacionesInvalidas.append(rel);
        }
        else if ((campoO.esPK && campoD.esPK) && (campoO.nombre != campoD.nombre)) {
            relacionesInvalidas.append(rel);
        }
        else if (!origenEsPK && !destinoEsPK) {
            // Esta sería una relación M:M potencial - validar con ValidadorRelaciones
            ValidadorRelaciones validador;
            if (!validador.puedeCrearRelacionMM(tablaOrigen, tablaDestino)) {
                // ❌ Cancelar relación y salir sin guardar
                relacionesInvalidas.append(rel);
                qDebug() << "❌ Relación M:M inválida entre" << tablaOrigen << "y" << tablaDestino
                         << "- al menos una tabla tiene PK, no se guardará en relationships.dat";
                break; // 🔴 salimos del for
            }
        }
        else {
            // Validar reglas de PK/FK según el tipo de relación
            bool esValida = true;
            switch (rel->getTipoRelacion()) {
            case TipoRelacion::UnoAUno:
                esValida = (origenEsPK && destinoEsPK);
                break;
            case TipoRelacion::UnoAMuchos:
                esValida = (origenEsPK && !destinoEsPK) || (!origenEsPK && destinoEsPK);
                break;
            case TipoRelacion::MuchosAMuchos:
                esValida = (!origenEsPK && !destinoEsPK);
                break;
            }
            if (!esValida) {
                relacionesInvalidas.append(rel);
            }
        }
    }

    // Eliminar relaciones inválidas sin mostrar popups
    for (RelationItem *rel : relacionesInvalidas) {
        eliminarRelacion(rel);
    }
}


void RelacionesWidget::eliminarRelacion(RelationItem *rel)
{
    if (!rel) return;

    // Eliminar de la escena
    scene->removeItem(rel);

    // Eliminar del vector
    relaciones.removeOne(rel);

    // Eliminar del archivo relationships.dat
    QFile relacionesFile("relationships.dat");
    if (relacionesFile.open(QIODevice::ReadOnly)) {
        QStringList lineasValidas;
        QTextStream in(&relacionesFile);
        while (!in.atEnd()) {
            QString linea = in.readLine();
            if (!linea.contains(rel->getSource()->getTableName() + "|" + rel->getCampoSource() + "|" +
                                rel->getDest()->getTableName() + "|" + rel->getCampoDest())) {
                lineasValidas << linea;
            }
        }
        relacionesFile.close();

        if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&relacionesFile);
            for (const QString &linea : lineasValidas)
                out << linea << "\n";
            relacionesFile.close();
        }
    }

    delete rel;
}

void RelacionesWidget::refrescarListaTablas()
{
    cargarListaTablas();
}
