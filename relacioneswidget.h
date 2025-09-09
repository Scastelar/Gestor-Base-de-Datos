#ifndef RELACIONESWIDGET_H
#define RELACIONESWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QListWidget>
#include "metadata.h"

class RelacionesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RelacionesWidget(QWidget *parent = nullptr);

signals:
    void cerrada();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void cargarListaTablas();                 // 🔹 cargar lista en el panel izquierdo
    void crearCardTabla(const Metadata &meta);// 🔹 crear card en el área central
    void crearToolbar();
    void crearLayoutPrincipal();

    QListWidget *listaTablas;    // 🔹 lista de tablas en el panel izquierdo
    QVBoxLayout *cardsLayout;    // layout para organizar las cards
};

#endif // RELACIONESWIDGET_H

