// TablaCentralWidget.h
#ifndef TABLACENTRALWIDGET_H
#define TABLACENTRALWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QIcon>

class TablaCentralWidget : public QWidget {
    Q_OBJECT

public:
    explicit TablaCentralWidget(QWidget *parent = nullptr);
    int obtenerFilaPK() const;
    void establecerPK();
    QString obtenerNombrePK() const;
    void actualizarPropiedades();

private:
    QTableWidget *tablaCampos;
    QTableWidget *tablaPropiedades;

    void configurarTablaCampos();
    void configurarTablaPropiedades();
    void agregarCampo();

    QString obtenerPropiedadesCampo(int row) const;
    void manejarCambioPK(QTableWidgetItem *item);
};

#endif // TABLACENTRALWIDGET_H
