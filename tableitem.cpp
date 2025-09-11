#include "tableitem.h"
#include "relationitem.h"
#include <QPainter>
#include <QFont>
#include <QGraphicsSceneMouseEvent>
#include <QGraphicsScene>

TableItem::TableItem(const Metadata &m, QGraphicsItem *parent)
    : QGraphicsObject(parent), meta(m), width(140)
{
    height = 25 + meta.campos.size() * 18;
    setFlags(ItemIsMovable | ItemIsSelectable | ItemSendsGeometryChanges);
}

QRectF TableItem::boundingRect() const
{
    return QRectF(0, 0, width, height);
}

void TableItem::paint(QPainter *p, const QStyleOptionGraphicsItem *, QWidget *)
{
    camposVisuales.clear();

    // Marco
    p->setBrush(Qt::white);
    p->setPen(QPen(QColor(43, 87, 154), 2));
    p->drawRect(boundingRect());

    // Cabecera
    p->setBrush(QColor(43, 87, 154));
    p->setPen(Qt::NoPen);
    p->drawRect(0, 0, width, 20);

    // Nombre tabla
    p->setPen(Qt::white);
    QFont font;
    font.setBold(true);
    font.setPointSize(8);
    p->setFont(font);
    p->drawText(QRect(0, 0, width, 20), Qt::AlignCenter, meta.nombreTabla);

    // Campos
    p->setPen(Qt::black);
    QFont f2;
    f2.setPointSize(8);
    p->setFont(f2);

    for (int i = 0; i < meta.campos.size(); i++) {
        const Campo &campo = meta.campos[i];
        QString text = campo.nombre + " (" + campo.tipo + ")";
        if (campo.esPK) text = "🔑 " + text;

        int y = 35 + i * 18;
        p->drawText(5, y, text);

        CampoVisual cv;
        cv.nombre = campo.nombre;
        cv.esPK = campo.esPK;
        cv.rect = QRectF(0, y - 12, width, 18);
        camposVisuales.append(cv);
    }
}

void TableItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    // ✅ Detectar doble click en un campo
    if (event->button() == Qt::LeftButton &&
        event->type() == QEvent::GraphicsSceneMouseDoubleClick) {
        QPointF pos = event->pos();
        for (const CampoVisual &cv : camposVisuales) {
            if (cv.rect.contains(pos)) {
                QPointF scenePos = mapToScene(cv.rect.center());
                emit iniciarDragCampo(meta.nombreTabla, cv.nombre, scenePos);

                // Bloquear movimiento durante el drag de relación
                setFlag(ItemIsMovable, false);
                event->accept();
                return;
            }
        }
    }

    // Normal → mover card
    QGraphicsObject::mousePressEvent(event);
}

void TableItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    // Restaurar movimiento de la card
    setFlag(ItemIsMovable, true);
    QGraphicsObject::mouseReleaseEvent(event);
}

QVariant TableItem::itemChange(GraphicsItemChange change, const QVariant &value)
{
    if (change == ItemPositionHasChanged && scene()) {
        for (QGraphicsItem *item : scene()->items()) {
            RelationItem *rel = dynamic_cast<RelationItem*>(item);
            if (rel && (rel->getSource() == this || rel->getDest() == this)) {
                rel->updatePosition();
            }
        }
    }
    return QGraphicsObject::itemChange(change, value);
}

