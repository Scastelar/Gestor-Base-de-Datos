
#include "relacionesview.h"
#include <QMouseEvent>

RelacionesView::RelacionesView(QWidget *parent)
    : QGraphicsView(parent)
{
}

void RelacionesView::mouseMoveEvent(QMouseEvent *event)
{
    emit mouseMovedEnScene(mapToScene(event->pos()));
    QGraphicsView::mouseMoveEvent(event);
}

void RelacionesView::mouseReleaseEvent(QMouseEvent *event)
{
    emit mouseReleasedEnScene(mapToScene(event->pos()));
    QGraphicsView::mouseReleaseEvent(event);
}
