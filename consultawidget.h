#ifndef CONSULTAWIDGET_H
#define CONSULTAWIDGET_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QListWidget>
#include <QStackedWidget>
#include <QPushButton>
#include <QMap>
#include "tableitem.h"
#include "metadata.h"
#include "querydesignerwidget.h"
#include "vistaconsulta.h"

class ConsultaWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ConsultaWidget(QWidget *parent = nullptr);

private slots:
    void agregarTabla();
    void ejecutarConsulta(const QString &sql);
    void volverADiseno();

private:
    // Navegación entre páginas
    QStackedWidget *stack;
    QWidget *paginaDiseno;
    QWidget *paginaResultado;

    // Diseño de consulta
    QGraphicsScene *scene;
    QGraphicsView *view;
    QListWidget *listaTablas;
    QueryDesignerWidget *gridDesigner;
    QMap<QString, TableItem*> tablas;
    QVector<Metadata> metasDisponibles; //  Metadatos cargados

    // Resultados
    VistaConsulta *vistaResultado;
    QPushButton *btnVolver;
};

#endif // CONSULTAWIDGET_H
