#ifndef RELATIONITEM_H
#define RELATIONITEM_H

#include <QGraphicsPathItem>
#include <QGraphicsSimpleTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QPen>
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
    QString getTipoRelacionString() const {
        switch (tipoRelacion) {
        case TipoRelacion::UnoAUno: return "Uno a Uno";
        case TipoRelacion::UnoAMuchos: return "Uno a Muchos";
        case TipoRelacion::MuchosAMuchos: return "Muchos a Muchos";
        default: return "Desconocido";
        }
    }

    TipoRelacion getTipoRelacion() const { return tipoRelacion; }

protected:
    // Permitir que se seleccione con clic
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

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
