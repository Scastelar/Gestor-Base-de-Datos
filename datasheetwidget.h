#ifndef DATASHEETWIDGET_H
#define DATASHEETWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QMap>
#include <QVariant>
#include "metadata.h"

class QTableWidget;
class QTableWidgetItem;
class QPushButton;

class DataSheetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataSheetWidget(QWidget *parent = nullptr);

    // 🔹 Nueva función: crear columnas según metadata
    void cargarDesdeMetadata(const Metadata &meta);

    // Métodos para manipular registros
    void agregarRegistro();
    void establecerPK();
    int obtenerFilaPK() const;
    QString obtenerNombrePK() const;

    // Método para obtener todos los registros
    QList<QMap<QString, QVariant>> obtenerRegistros(const QVector<Campo> &campos) const;

    // Métodos auxiliares
    int obtenerUltimoID() const { return ultimoID; }
    int obtenerCantidadRegistros() const { return tablaRegistros->rowCount(); }

    void configurarEditorFecha(QTableWidgetItem *item, const QString &formato);
    QString formatearFecha(const QDateTime &fecha, const QString &formato) const;
    void mostrarSelectorFecha(int row, int col, const QString &formato);
    QString formatearFechaSegunFormato(const QDate &fecha, const QString &formato) const;
    void onCellDoubleClicked(int row, int column);

signals:
    void registroAgregado(int id, const QString &campo1);
    void registroModificado(int fila);

private slots:
    void onCellChanged(int row, int column);
    void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void onCellChangedValidacion(int row, int column); // AÑADIDO: Declaración faltante

public slots:
    void validarRegistroCompleto(int fila);

private:
    void configurarTablaRegistros();
    void actualizarAsteriscoIndice(int nuevaFila, int viejaFila);
    void resaltarErrores(int fila, bool tieneErrores);
    bool esRegistroValido(int fila);

    QTableWidget *tablaRegistros;
    int indiceActual;
    QVector<Campo> camposMetadata;
    int ultimoID; // Contador para IDs automáticos

    bool validarLlavePrimariaUnica(int filaActual);
    bool validarTipoDato(int fila, int columna, const QString &valor);
    bool esValorUnicoEnColumna(int columna, const QString &valor, int filaExcluir = -1);

};

#endif // DATASHEETWIDGET_H
