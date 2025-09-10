#ifndef RELACIONESWIDGET_H
#define RELACIONESWIDGET_H

#include "metadata.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsLineItem>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QMouseEvent>

class RelacionesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RelacionesWidget(QWidget *parent = nullptr);
    ~RelacionesWidget();

signals:
    void cerrada();
    void relacionCreada(const QString &tabla1, const QString &campo1,
                        const QString &tabla2, const QString &campo2);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
     void resizeEvent(QResizeEvent *event) override;
private:

    void cargarListaTablas();                 // 🔹 cargar lista en el panel izquierdo
    void crearCardTabla(const Metadata &meta);// 🔹 crear card en el área central
    void crearToolbar();
    void crearLayoutPrincipal();

    QListWidget *listaTablas;    // 🔹 lista de tablas en el panel izquierdo
    QVBoxLayout *cardsLayout;    // layout para organizar las cards

    // Nuevos miembros para el sistema de relaciones
    QGraphicsScene *scene;
    QGraphicsView *view;
    QMap<QString, QGraphicsItemGroup*> tablaItems;

    QMap<QString, QMap<QString, QGraphicsTextItem*>> campoItems;

    QString tablaOrigen;
    QString campoOrigen;
    bool seleccionandoOrigen;

    QList<QGraphicsLineItem*> lineasRelaciones;

    void agregarTablaAScene(const Metadata &meta);


    void crearSistemaRelaciones();
    void procesarClickTabla(const QString &tablaNombre, const QString &campoNombre);
    void dibujarRelacion(const QString &tabla1, const QString &campo1,
                         const QString &tabla2, const QString &campo2);
    void mostrarInfoCampo(const Metadata &meta, QLabel *labelCampo, const Campo &campo);
};

#endif // RELACIONESWIDGET_H

