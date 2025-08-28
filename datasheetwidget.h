#ifndef DATASHEETWIDGET_H
#define DATASHEETWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QMap>
#include <QVariant>

class QTableWidget;
class QTableWidgetItem;
class QPushButton;

class DataSheetWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DataSheetWidget(QWidget *parent = nullptr);

    // Métodos para manipular registros
    void agregarRegistro();
    void establecerPK();
    int obtenerFilaPK() const;
    QString obtenerNombrePK() const;

    // Método para obtener todos los registros
    QList<QMap<QString, QVariant>> obtenerRegistros() const;

    // Métodos para obtener datos específicos
    int obtenerUltimoID() const { return ultimoID; }
    int obtenerCantidadRegistros() const { return tablaRegistros->rowCount(); }

    void actualizarPropiedades();

signals:
    void registroAgregado(int id, const QString &campo1);
    void registroModificado(int fila);

private slots:
    void onCellChanged(int row, int column);
    void onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn);

private:
    void configurarTablaRegistros();
    void actualizarAsteriscoIndice(int nuevaFila, int viejaFila);

    QTableWidget *tablaRegistros;
    int indiceActual;
    int ultimoID; // Contador para IDs automáticos
};

#endif // DATASHEETWIDGET_H
