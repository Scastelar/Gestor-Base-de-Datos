#include "relationitem.h"
#include <QPen>
#include <QPainterPath>
#include <QFont>

RelationItem::RelationItem(TableItem *src, const QString &campoSrc,
                           TableItem *dst, const QString &campoDst,
                           TipoRelacion tipo)
    : source(src), dest(dst),
    campoSource(campoSrc), campoDest(campoDst),
    tipoRelacion(tipo)
{
    QPen pen(Qt::black, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    setPen(pen);

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setAcceptedMouseButtons(Qt::LeftButton);

    labelSrc = new QGraphicsSimpleTextItem(this);
    labelDst = new QGraphicsSimpleTextItem(this);

    configurarEtiquetas();
    updatePosition();
}

void RelationItem::configurarEtiquetas()
{
    switch (tipoRelacion) {
    case TipoRelacion::UnoAUno:
        labelSrc->setText("1");
        labelDst->setText("1");
        break;
    case TipoRelacion::UnoAMuchos:
        labelSrc->setText("1");
        labelDst->setText("∞");
        break;
    case TipoRelacion::MuchosAMuchos:
        labelSrc->setText("∞");
        labelDst->setText("∞");
        break;
    }
    QFont font;
    font.setPointSize(10);
    font.setBold(true);
    labelSrc->setFont(font);
    labelDst->setFont(font);
}

void RelationItem::updatePosition()
{
    if (!source || !dest) return;

    QPointF p1 = source->mapToScene(source->boundingRect().center());
    QPointF p2 = dest->mapToScene(dest->boundingRect().center());

    for (const CampoVisual &cv : source->getCamposVisuales()) {
        if (cv.nombre == campoSource) {
            QRectF r = cv.rect;
            // salir por el borde derecho (junto al "1")
            p1 = source->mapToScene(r.right(), r.center().y());
            break;
        }
    }
    for (const CampoVisual &cv : dest->getCamposVisuales()) {
        if (cv.nombre == campoDest) {
            QRectF r = cv.rect;
            // entrar por el borde izquierdo (junto al "∞")
            p2 = dest->mapToScene(r.left(), r.center().y());
            break;
        }
    }


    QPainterPath path;
    path.moveTo(p1);
    QPointF ctrl1(p1.x() + 40, p1.y());
    QPointF ctrl2(p2.x() - 40, p2.y());
    path.cubicTo(ctrl1, ctrl2, p2);
    setPath(path);

    //  Reubicar etiquetas al lado del campo (no encima del texto)
    for (const CampoVisual &cv : source->getCamposVisuales()) {
        if (cv.nombre == campoSource) {
            QRectF r = cv.rect;
            QPointF pos = source->mapToScene(r.right(), r.center().y());
            labelSrc->setPos(pos.x() + 5, pos.y() - 8); // a la derecha del campo
            break;
        }
    }
    for (const CampoVisual &cv : dest->getCamposVisuales()) {
        if (cv.nombre == campoDest) {
            QRectF r = cv.rect;
            QPointF pos = dest->mapToScene(r.left(), r.center().y());
            labelDst->setPos(pos.x() - 15, pos.y() - 8); // a la izquierda del campo
            break;
        }
    }

}
void RelationItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    QPen pen = this->pen();
    if (isSelected()) {
        // Deseleccionar → volver a negro
        pen.setColor(Qt::black);
        setSelected(false);
    } else {
        // Seleccionar → amarillo
        pen.setColor(Qt::yellow);
        setSelected(true);
    }
    setPen(pen);
    QGraphicsPathItem::mousePressEvent(event);
}


