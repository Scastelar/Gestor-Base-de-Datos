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

    labelSrc->setPos(p1.x() - 10, p1.y() - 10);
    labelDst->setPos(p2.x() - 10, p2.y() - 10);
}
