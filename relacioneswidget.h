#ifndef RELACIONESWIDGET_H
#define RELACIONESWIDGET_H

#include <QWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QListWidget>
#include <QMap>
#include "metadata.h"
#include "tableitem.h"
#include "relationitem.h"

class RelacionesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RelacionesWidget(QWidget *parent = nullptr);
    ~RelacionesWidget();

signals:
    void cerrada();
    void relacionCreada(const QString &tabla1,
                        const QString &campo1,
                        const QString &tabla2,
                        const QString &campo2);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void agregarTabla();
    void limpiarTodo();

private:
    void crearToolbar();
    void crearLayoutPrincipal();
    void cargarListaTablas();

    QGraphicsScene *scene;
    QGraphicsView *view;
    QListWidget *listaTablas;

    QMap<QString, TableItem*> tablas;
    QList<RelationItem*> relaciones;

    QString tablaOrigen;
    QString campoOrigen;
    bool esperandoDestino = false;
};

#endif // RELACIONESWIDGET_H
