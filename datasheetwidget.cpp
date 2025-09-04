#include "DataSheetWidget.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>

DataSheetWidget::DataSheetWidget(QWidget *parent)
    : QWidget(parent), indiceActual(-1), ultimoID(0)
{
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(this);

    // Crear tabla para registros
    tablaRegistros = new QTableWidget(1, 3, this); // 3 columnas: *, ID, Campo1
    configurarTablaRegistros();

    // Headers simplificados
    QStringList headers = {"*", "ID", "Campo1"};
    tablaRegistros->setHorizontalHeaderLabels(headers);

    // Ajustes de estilo
    tablaRegistros->horizontalHeader()->setStretchLastSection(true);
    tablaRegistros->verticalHeader()->setVisible(true);
    tablaRegistros->setSelectionBehavior(QAbstractItemView::SelectItems);
    tablaRegistros->setSelectionMode(QAbstractItemView::SingleSelection);
    tablaRegistros->setAlternatingRowColors(true);
    tablaRegistros->setStyleSheet(
        "QTableWidget {"
        "   gridline-color: #d0d0d0;"
        "   background-color: white;"
        "}"
        "QTableWidget::item:selected {"
        "   background-color: #0078d4;"
        "   color: white;"
        "}"
        "QHeaderView::section {"
        "   background-color: #f0f0f0;"
        "   padding: 4px;"
        "   border: 1px solid #d0d0d0;"
        "   font-weight: bold;"
        "}"
        );

    // Conectar señales
    connect(tablaRegistros, &QTableWidget::cellChanged, this, &DataSheetWidget::onCellChanged);
    connect(tablaRegistros, &QTableWidget::currentCellChanged, this, &DataSheetWidget::onCurrentCellChanged);

    // Botón para agregar registro
    QPushButton *btnAgregar = new QPushButton("Agregar registro");
    connect(btnAgregar, &QPushButton::clicked, this, &DataSheetWidget::agregarRegistro);

    layoutPrincipal->addWidget(tablaRegistros);
    layoutPrincipal->addWidget(btnAgregar);

    // Configurar fila inicial
    agregarRegistro();
}

void DataSheetWidget::configurarTablaRegistros()
{
    // Configurar columnas
    tablaRegistros->setColumnWidth(0, 30);  // Columna del asterisco
    tablaRegistros->setColumnWidth(1, 80);  // Columna del ID
    tablaRegistros->setColumnWidth(2, 200); // Columna Campo1

    // Columna del asterisco (no editable)
    QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
    asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
    asteriscoItem->setTextAlignment(Qt::AlignCenter);
    tablaRegistros->setItem(0, 0, asteriscoItem);

    // Columna ID (no editable, numérico automático)
    QTableWidgetItem *idItem = new QTableWidgetItem();
    idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setText(QString::number(++ultimoID));
    tablaRegistros->setItem(0, 1, idItem);

    // Columna Campo1
    QTableWidgetItem *campo1Item = new QTableWidgetItem("Valor");
    tablaRegistros->setItem(0, 2, campo1Item);

    // Marcar la fila actual con asterisco
    indiceActual = 0;
    QTableWidgetItem *currentAsterisco = tablaRegistros->item(0, 0);
    if (currentAsterisco) {
        currentAsterisco->setText("*");
        currentAsterisco->setToolTip("Fila actual");
    }
}

void DataSheetWidget::agregarRegistro()
{
    int row = tablaRegistros->rowCount();
    tablaRegistros->insertRow(row);

    // Columna del asterisco (no editable)
    QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
    asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
    asteriscoItem->setTextAlignment(Qt::AlignCenter);
    tablaRegistros->setItem(row, 0, asteriscoItem);

    // Columna ID (no editable, numérico automático)
    QTableWidgetItem *idItem = new QTableWidgetItem();
    idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
    idItem->setTextAlignment(Qt::AlignCenter);
    idItem->setText(QString::number(++ultimoID));
    tablaRegistros->setItem(row, 1, idItem);

    // Columna Campo1
    QTableWidgetItem *campo1Item = new QTableWidgetItem("Valor");
    tablaRegistros->setItem(row, 2, campo1Item);

    emit registroAgregado(ultimoID, "Valor");
}

void DataSheetWidget::actualizarAsteriscoIndice(int nuevaFila, int viejaFila)
{
    // Quitar asterisco de la fila anterior
    if (viejaFila >= 0 && viejaFila < tablaRegistros->rowCount()) {
        QTableWidgetItem *oldAsterisco = tablaRegistros->item(viejaFila, 0);
        if (oldAsterisco) {
            oldAsterisco->setText("");
        }
    }

    // Poner asterisco en la nueva fila actual
    if (nuevaFila >= 0 && nuevaFila < tablaRegistros->rowCount()) {
        QTableWidgetItem *newAsterisco = tablaRegistros->item(nuevaFila, 0);
        if (newAsterisco) {
            newAsterisco->setText("*");
            newAsterisco->setToolTip("Fila actual");
        }
        indiceActual = nuevaFila;
    }
}

void DataSheetWidget::onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousColumn);

    actualizarAsteriscoIndice(currentRow, previousRow);
}

void DataSheetWidget::onCellChanged(int row, int column)
{
    if (column == 2) { // Si cambió el valor de Campo1
        QTableWidgetItem *item = tablaRegistros->item(row, 2);
        if (item && item->text().isEmpty()) {
            item->setText("Valor");
        }
        emit registroModificado(row);
    }
}

void DataSheetWidget::establecerPK()
{
    int currentRow = indiceActual;
    if (currentRow == -1) {
        QMessageBox::information(this, "Selección requerida", "Por favor, seleccione una fila primero.");
        return;
    }

    // Quitar PK de cualquier otra fila
    for (int row = 0; row < tablaRegistros->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaRegistros->item(row, 0);
        if (pkItem && pkItem->toolTip().contains("Llave Primaria")) {
            // Mantener el asterisco si es la fila actual, sino limpiar
            if (row == indiceActual) {
                pkItem->setText("*");
            } else {
                pkItem->setText("");
            }
            pkItem->setToolTip("");
        }
    }

    // Establecer PK en la fila seleccionada
    QTableWidgetItem *pkItem = tablaRegistros->item(currentRow, 0);
    if (pkItem) {
        pkItem->setText("🔑");
        pkItem->setToolTip("Llave Primaria");
    }
}

int DataSheetWidget::obtenerFilaPK() const
{
    for (int row = 0; row < tablaRegistros->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaRegistros->item(row, 0);
        if (pkItem && pkItem->toolTip().contains("Llave Primaria")) {
            return row;
        }
    }
    return -1;
}

QString DataSheetWidget::obtenerNombrePK() const
{
    int pkRow = obtenerFilaPK();
    if (pkRow != -1) {
        QTableWidgetItem *idItem = tablaRegistros->item(pkRow, 1);
        if (idItem) {
            return idItem->text();
        }
    }
    return "";
}

QList<QMap<QString, QVariant>> DataSheetWidget::obtenerRegistros(const QVector<Campo> &campos) const
{
    QList<QMap<QString, QVariant>> registros;

    for (int row = 0; row < tablaRegistros->rowCount(); ++row) {
        QMap<QString, QVariant> registro;

        for (int col = 0; col < campos.size(); ++col) {
            const Campo &campo = campos[col];
            QVariant valor;

            QWidget *editor = tablaRegistros->cellWidget(row, col + 2); // +2 por *, ID

            if (campo.tipo == "TEXTO") {
                QLineEdit *lineEdit = qobject_cast<QLineEdit*>(editor);
                if (lineEdit) {
                    QString texto = lineEdit->text();
                    int maxLen = campo.obtenerPropiedad().toInt();
                    if (texto.size() > maxLen) {
                        texto = texto.left(maxLen);
                    }
                    valor = texto;
                }
            }
            else if (campo.tipo == "NUMERO") {
                QLineEdit *edit = qobject_cast<QLineEdit*>(editor);
                if (edit) {
                    valor = edit->text().toDouble();
                }
            }
            else if (campo.tipo == "MONEDA") {
                QDoubleSpinBox *doubleSpin = qobject_cast<QDoubleSpinBox*>(editor);
                if (doubleSpin) {
                    valor = doubleSpin->value();
                }
            }
            else if (campo.tipo == "FECHA") {
                QDateTimeEdit *dateEdit = qobject_cast<QDateTimeEdit*>(editor);
                if (dateEdit) {
                    valor = dateEdit->dateTime();
                }
            }

            registro[campo.nombre] = valor;
        }

        registros.append(registro);
    }

    return registros;
}


void DataSheetWidget::cargarDesdeMetadata(const Metadata &meta)
{
    tablaRegistros->clear();
    tablaRegistros->setRowCount(0);
    tablaRegistros->setColumnCount(meta.campos.size() + 2);

    QStringList headers;
    headers << "*" << "ID";
    for (const Campo &c : meta.campos) {
        headers << c.nombre;
    }
    tablaRegistros->setHorizontalHeaderLabels(headers);

    ultimoID = 0;
    indiceActual = -1;

    // 🔹 Cargar registros desde Metadata
    for (const auto &registro : meta.registros) {
        int row = tablaRegistros->rowCount();
        tablaRegistros->insertRow(row);

        // Asterisco
        QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
        asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
        tablaRegistros->setItem(row, 0, asteriscoItem);

        // ID
        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(++ultimoID));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        tablaRegistros->setItem(row, 1, idItem);

        // Campos
        for (int i = 0; i < meta.campos.size(); i++) {
            const Campo &campo = meta.campos[i];
            QVariant valor = registro.value(campo.nombre);

            if (campo.tipo == "TEXTO") {
                QLineEdit *edit = new QLineEdit(valor.toString());
                edit->setMaxLength(campo.obtenerPropiedad().toInt());
                tablaRegistros->setCellWidget(row, i + 2, edit);
            }
            else if (campo.tipo == "NUMERO") {
                QLineEdit *edit = new QLineEdit(QString::number(valor.toDouble()));
                edit->setValidator(new QDoubleValidator(edit));
                tablaRegistros->setCellWidget(row, i + 2, edit);
            }
            else if (campo.tipo == "MONEDA") {
                QDoubleSpinBox *spin = new QDoubleSpinBox();
                spin->setPrefix(campo.obtenerPropiedad().toString() + " ");
                spin->setMaximum(1e9);
                spin->setValue(valor.toDouble());
                tablaRegistros->setCellWidget(row, i + 2, spin);
            }
            else if (campo.tipo == "FECHA") {
                QDateTimeEdit *edit = new QDateTimeEdit();
                edit->setDisplayFormat(campo.obtenerPropiedad().toString());
                edit->setDateTime(valor.toDateTime());
                tablaRegistros->setCellWidget(row, i + 2, edit);
            }
        }
    }
}





