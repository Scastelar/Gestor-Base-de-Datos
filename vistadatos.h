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
    void cargarDesdeMetadata(const Metadata &meta);
    void agregarRegistro();
    void eliminarRegistro();
    void establecerPK();
    int obtenerFilaPK() const;
    QString obtenerNombrePK() const;
    QList<QMap<QString, QVariant>> obtenerRegistros(const QVector<Campo> &campos) const;
    int obtenerUltimoID() const { return ultimoID; }
    int obtenerCantidadRegistros() const { return tablaRegistros->rowCount(); }

    void ordenar(Qt::SortOrder order);

    void mostrarSelectorFecha(int row, int col, const QString &formato);
    QString formatearFecha(const QVariant &fechaInput, const QString &formato) const;
    QString formatearMoneda(double valor, const QString &simbolo) const;
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
    void onCellDoubleClicked(int row, int column);
    void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);
    void onBotonRelacionClicked(int fila, int columna);
    void expandirContraerRelacion(int fila, int columna);

private:
    QTableWidget *tablaRegistros;
    QTableWidget *tablaRelacionada;
    QWidget *contenedorRelacionado;
    int indiceActual;
    int ultimoID;
    int filaExpandida;
    bool relacionExpandida;
    QVector<Campo> camposMetadata;
    QList<QMap<QString, QString>> relaciones;
    QMap<int, QMap<int, QPushButton*>> botonesRelaciones;

    void configurarTablaRegistros();
    void actualizarAsteriscoIndice(int nuevaFila, int viejaFila);
    void resaltarErrores(int fila, bool tieneErrores);
    bool esRegistroValido(int fila);
    void agregarBotonRelacion(int fila, int columna);
    QMap<QString, QString> obtenerRelacionParaCampo(const QString &nombreCampo) const;
    bool esRelacionMuchosAMuchos(const QMap<QString, QString> &relacion) const;
    void configurarCelda(int fila, int columna, const QVariant &valor, const Campo &campo);
    void configurarCeldaTexto(QTableWidgetItem *item, const QVariant &valor, const Campo &campo);
    void configurarCeldaMoneda(QTableWidgetItem *item, const QVariant &valor, const Campo &campo);
    void configurarCeldaFecha(QTableWidgetItem *item, const QVariant &valor, const Campo &campo);
    void configurarCeldaNumero(QTableWidgetItem *item, const QVariant &valor, const Campo &campo);
    bool validarLlavePrimariaUnica(int filaActual);
    bool validarTipoDato(int fila, int columna, const QString &valor);
    bool esValorUnicoEnColumna(int columna, const QString &valor, int filaExcluir = -1);
};

#endif // VISTADATOS_H
