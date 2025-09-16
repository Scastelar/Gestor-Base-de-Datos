#ifndef RELACIONESWIDGET_H
#define RELACIONESWIDGET_H

#include <QWidget>
#include <QGraphicsScene>
#include <QListWidget>
#include <QMap>
#include "metadata.h"
#include "tableitem.h"
#include "relationitem.h"
#include "relacionesview.h"

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
    void eliminarRelacionSeleccionada();
    bool validarCompatibilidadTipos(const Campo &campoOrigen, const Campo &campoDestino);
    void crearToolbar();
    void crearLayoutPrincipal();
    void cargarListaTablas();
    void cargarRelacionesPrevias(); // 🔹 nuevo método

    QGraphicsScene *scene;
    RelacionesView *view;
    QListWidget *listaTablas;

    QMap<QString, TableItem*> tablas;
    QList<RelationItem*> relaciones;

    // Drag temporal
    QGraphicsLineItem *lineaTemporal = nullptr;
    QString tablaDrag;
    QString campoDrag;
    QPointF puntoDrag;
    bool arrastrando = false;
};

#endif // RELACIONESWIDGET_H
