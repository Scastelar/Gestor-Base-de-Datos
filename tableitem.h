#ifndef TABLEITEM_H
#define TABLEITEM_H

#include <QGraphicsObject>
#include <QVector>
#include "metadata.h"

struct CampoVisual {
    QString nombre;
    QRectF rect;   // área clicable dentro de la card
    bool esPK;
};

class TableItem : public QGraphicsObject
{
    Q_OBJECT
public:
    explicit TableItem(const Metadata &meta, QGraphicsItem *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter,
               const QStyleOptionGraphicsItem *option,
               QWidget *widget = nullptr) override;

    QString getTableName() const { return meta.nombreTabla; }
    QVector<CampoVisual> getCamposVisuales() const { return camposVisuales; }

signals:
    void campoSeleccionado(const QString &tabla,
                           const QString &campo,
                           const QPointF &posScene);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    QVariant itemChange(GraphicsItemChange change,
                        const QVariant &value) override;

private:
    Metadata meta;
    int width;
    int height;
    QVector<CampoVisual> camposVisuales;
};

#endif // TABLEITEM_H
