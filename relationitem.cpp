#include "relationitem.h"
#include <QPen>
#include <QPainterPath>

RelationItem::RelationItem(TableItem *src, const QString &campoSrc,
                           TableItem *dst, const QString &campoDst,
                           TipoRelacion tipo)
    : source(src), dest(dst),
    campoSource(campoSrc), campoDest(campoDst),
    tipoRelacion(tipo)
{
    aplicarEstilo();
    updatePosition();
}

void RelationItem::updatePosition()
{
    if (!source || !dest) return;

    QPointF p1, p2;

    for (const CampoVisual &cv : source->getCamposVisuales()) {
        if (cv.nombre == campoSource) {
            p1 = source->mapToScene(cv.rect.center());
            break;
        }
    }
    for (const CampoVisual &cv : dest->getCamposVisuales()) {
        if (cv.nombre == campoDest) {
            p2 = dest->mapToScene(cv.rect.center());
            break;
        }
    }

    QPainterPath path;
    path.moveTo(p1);
    QPointF ctrl1(p1.x() + 40, p1.y());
    QPointF ctrl2(p2.x() - 40, p2.y());
    path.cubicTo(ctrl1, ctrl2, p2);

    setPath(path);
}

void RelationItem::aplicarEstilo()
{
    QPen pen;
    pen.setWidth(2);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);

    switch (tipoRelacion) {
    case TipoRelacion::UnoAUno:
        pen.setColor(Qt::blue);
        break;
    case TipoRelacion::UnoAMuchos:
        pen.setColor(Qt::darkGreen);
        break;
    case TipoRelacion::MuchosAMuchos:
        pen.setColor(Qt::red);
        pen.setStyle(Qt::DashLine);
        break;
    }
    setPen(pen);
}
