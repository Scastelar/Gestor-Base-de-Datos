#ifndef VISTACONSULTA_H
#define VISTACONSULTA_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include "metadata.h"

class VistaConsulta : public QWidget
{
    Q_OBJECT
public:
    explicit VistaConsulta(QWidget *parent = nullptr);

    void mostrarConsulta(const QString &sqlLike,
                         const QVector<Metadata> &tablas);

private:
    QTableWidget *tablaResultados;

    void aplicarConsulta(const QString &sqlLike,
                         const QVector<Metadata> &tablas);
};

#endif // VISTACONSULTA_H
