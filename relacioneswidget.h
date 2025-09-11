#ifndef RELACIONESWIDGET_H
#define RELACIONESWIDGET_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QMap>
#include <QList>
#include <QListWidget>

#include "metadata.h"

class RelacionesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RelacionesWidget(QWidget *parent = nullptr);
    ~RelacionesWidget();

signals:
    void cerrada();
    void relacionCreada(const QString &tablaOrigen, const QString &campoOrigen,
                        const QString &tablaDestino, const QString &campoDestino,
                        const QString &tipoRelacion);

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void actualizarLineas();

private:
    void crearToolbar();
    void crearLayoutPrincipal();
    void crearSistemaRelaciones();
    void cargarListaTablas();
    void agregarTablaAScene(const Metadata &meta);
    void procesarDragAndDrop(const QString &tablaOrigen, const QString &campoOrigen,
                             const QString &tablaDestino, const QString &campoDestino);
    void dibujarRelacion(const QString &tabla1, const QString &campo1,
                         const QString &tabla2, const QString &campo2,
                         const QString &tipoRelacion);
    QString determinarTipoRelacion(const QString &tabla1, const QString &campo1,
                                   const QString &tabla2, const QString &campo2);
    void mostrarMensajePersonalizado(const QString &titulo, const QString &mensaje, bool esError = false);

    QGraphicsScene *scene;
    QGraphicsView *view;
    QListWidget *listaTablas;

    QMap<QString, QGraphicsItemGroup*> tablaItems;
    QMap<QString, QMap<QString, QGraphicsTextItem*>> campoItems;
    QMap<QPair<QString, QString>, QGraphicsLineItem*> lineasRelaciones;
    QMap<QGraphicsLineItem*, QPair<QString, QString>> relacionInfo;

    bool arrastrando;
    QString tablaArrastre;
    QString campoArrastre;
    QGraphicsTextItem *campoArrastreItem;
    QPointF posicionInicialArrastre;
};

#endif // RELACIONESWIDGET_H
