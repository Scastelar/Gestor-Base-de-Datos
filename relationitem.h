#ifndef RELATIONITEM_H
#define RELATIONITEM_H

#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include "tableitem.h"

enum class TipoRelacion {
    UnoAUno,
    UnoAMuchos,
    MuchosAMuchos
};

class RelationItem : public QGraphicsPathItem
{
public:
    RelationItem(TableItem *src, const QString &campoSrc,
                 TableItem *dst, const QString &campoDst,
                 TipoRelacion tipo = TipoRelacion::UnoAMuchos);

    void updatePosition();

    TableItem* getSource() const { return source; }
    TableItem* getDest() const { return dest; }
    QString getCampoSource() const { return campoSource; }
    QString getCampoDest() const { return campoDest; }

private:
    TableItem *source;
    TableItem *dest;
    QString campoSource;
    QString campoDest;
    TipoRelacion tipoRelacion;

    QGraphicsSimpleTextItem *labelSrc;
    QGraphicsSimpleTextItem *labelDst;

    void configurarEtiquetas();
};

#endif // RELATIONITEM_H
