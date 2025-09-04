#ifndef TABLACENTRALWIDGET_H
#define TABLACENTRALWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include "metadata.h"   // 🔹 Para usar struct Campo

class TablaCentralWidget : public QWidget {
    Q_OBJECT

public:
    explicit TablaCentralWidget(QWidget *parent = nullptr);
    bool validarPK() const;
    // 🔹 Manejo de la PK
    int obtenerFilaPK() const;
    void establecerPK();
    QString obtenerNombrePK() const;

    // 🔹 Manejo de propiedades de campo
    void actualizarPropiedades();
    QString obtenerPropiedadesCampo(int row) const;

    // 🔹 Guardado / carga de metadata
    QVector<Campo> obtenerCampos() const;
    void cargarCampos(const QVector<Campo>& campos);
    void eliminarCampo();
    void agregarCampo();
private slots:


private:
    QTableWidget *tablaCampos;
    QTableWidget *tablaPropiedades;

    void configurarTablaCampos();
    void configurarTablaPropiedades();
    void manejarCambioPK(QTableWidgetItem *item);
};

#endif // TABLACENTRALWIDGET_H
