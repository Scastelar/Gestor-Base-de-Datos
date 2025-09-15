#ifndef VISTADATOS_H
#define VISTADATOS_H

#include <QWidget>
#include <QTableWidget>
#include <QMap>
#include <QVariant>
#include "metadata.h"

class QTableWidget;
class QTableWidgetItem;
class QPushButton;
class QLabel;

class VistaDatos : public QWidget
{
    Q_OBJECT

public:
    explicit VistaDatos(QWidget *parent = nullptr);

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
    QString formatearMoneda(double valor, const QString &simbolo) const;
    void mostrarSelectorFecha(int row, int col, const QString &formato);
    QString formatearFechaSegunFormato(const QDate &fecha, const QString &formato) const;
    void onCellDoubleClicked(int row, int column);

    // 🔹 Nuevo: Cargar relaciones desde archivo
    void cargarRelaciones(const QString &archivoRelaciones);
    void mostrarTablaRelacionada(const QString &tablaDestino, const QString &campoOrigen, const QString &valor);

signals:
    void solicitarDatosRelacionados(const QString &tablaDestino, const QString &campoOrigen, const QString &valor);
    void registroAgregado(int id, const QString &campo1);
    void registroModificado(int fila);
    void solicitarTablaRelacionada(const QString &tablaRelacionada, const QString &campoOrigen, const QString &valorOrigen);

public slots:
    void validarRegistroCompleto(int fila);
    void onDatosRelacionadosRecibidos(const QList<QMap<QString, QVariant>> &datos);

private slots:
    void onCellChanged(int row, int column);
    void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void onCellChangedValidacion(int row, int column);
    void onBotonRelacionClicked(int fila, int columna); // 🔹 Cambiado: sin parámetro Relacion
    void expandirContraerRelacion(int fila, int columna);

private:
    QTableWidget *tablaRelacionada;
    QWidget *contenedorRelacionado;
    int filaExpandida;
    bool relacionExpandida;

    void configurarTablaRegistros();
    void actualizarAsteriscoIndice(int nuevaFila, int viejaFila);
    void resaltarErrores(int fila, bool tieneErrores);
    bool esRegistroValido(int fila);
    void agregarBotonRelacion(int fila, int columna);
    QMap<QString, QString> obtenerRelacionParaCampo(const QString &nombreCampo) const; // 🔹 Nuevo método
    bool esRelacionMuchosAMuchos(const QMap<QString, QString> &relacion) const;

    QTableWidget *tablaRegistros;
    int indiceActual;
    QVector<Campo> camposMetadata;
    int ultimoID; // Contador para IDs automáticos

    // 🔹 Cambiado: Ahora usamos QMap para almacenar relaciones
    QList<QMap<QString, QString>> relaciones; // Lista de mapas con información de relaciones
    QMap<int, QMap<int, QPushButton*>> botonesRelaciones; // 🔹 Mapa de botones por fila/columna

    bool validarLlavePrimariaUnica(int filaActual);
    bool validarTipoDato(int fila, int columna, const QString &valor);
    bool esValorUnicoEnColumna(int columna, const QString &valor, int filaExcluir = -1);

};

#endif // VISTADATOS_H
