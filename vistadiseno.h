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

class VistaDiseno : public QWidget {
    Q_OBJECT

public:
    explicit VistaDiseno(QWidget *parent = nullptr);
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
    void guardarPropiedadesActuales();
    void on_tablaCampos_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void on_tablaCampos_cellChanged(int row, int column);
    void guardarPropiedadFila(int row);

    // 🔹 Nuevos métodos para manejar campos relacionados
    void setCamposRelacionados(const QSet<QString>& camposRelacionados);
    void setNombreTabla(const QString& nombre);
    void guardarMetadatos(); // Guardar automáticamente
    void onCellDoubleClicked(int row, int column);
signals:
    void metadatosModificados();

private slots:

    void on_campoEditado(QTableWidgetItem *item);


private:
    QTableWidget *tablaCampos;
    QTableWidget *tablaPropiedades;
    QMap<int, QVariant> propiedadesPorFila;
    QSet<QString> camposRelacionados; // Campos que tienen relaciones
    QString nombreTablaActual;

    QMap<int, QString> nombresAnteriores;


    void configurarTablaCampos();
    void configurarTablaPropiedades();
    void manejarCambioPK(QTableWidgetItem *item);

    bool esCampoRelacionado(const QString& nombreCampo) const;
    void actualizarEstadoCampos(); // Actualizar estado editable de campos

    bool guardandoMetadatos;
    bool bloqueandoEdicion;
};

#endif // TABLACENTRALWIDGET_H
