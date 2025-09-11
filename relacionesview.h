#ifndef RELACIONESVIEW_H
#define RELACIONESVIEW_H

#include <QGraphicsView>

class RelacionesView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit RelacionesView(QWidget *parent = nullptr);

signals:
    void mouseMovedEnScene(const QPointF &pos);
    void mouseReleasedEnScene(const QPointF &pos);

protected:
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
};

#endif // RELACIONESVIEW_H
