#include "tablacentralwidget.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>

//Comentario para que me deje push
TablaCentralWidget::TablaCentralWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(this);

    // Crear tabla
    tablaCampos = new QTableWidget(1, 3, this);
    configurarTablaCampos();
    tablaCampos->setColumnCount(3); // PK, Nombre, Tipo de dato
    QStringList headers = {"", "Nombre del Campo", "Tipo de Dato"}; // PK vacío
    tablaCampos->setHorizontalHeaderLabels(headers);

    // Ajustes de estilo
    tablaCampos->horizontalHeader()->setStretchLastSection(true);
    tablaCampos->verticalHeader()->setVisible(false);
    tablaCampos->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaCampos->setSelectionMode(QAbstractItemView::SingleSelection);
    tablaCampos->setAlternatingRowColors(true);

    // Conectar señal de cambio de selección
    connect(tablaCampos, &QTableWidget::itemSelectionChanged, this, &TablaCentralWidget::actualizarPropiedades);

    // 🔹 Botón para agregar campo
    QPushButton *btnAgregar = new QPushButton("Agregar campo");
    connect(btnAgregar, &QPushButton::clicked, this, &TablaCentralWidget::agregarCampo);

    tablaPropiedades = new QTableWidget(0, 2, this);
    configurarTablaPropiedades();

    layoutPrincipal->addWidget(tablaCampos);
    layoutPrincipal->addWidget(tablaPropiedades);
    layoutPrincipal->addWidget(btnAgregar);
}


void TablaCentralWidget::configurarTablaCampos() {
    tablaCampos->setStyleSheet(
        "QTableWidget::item:selected { "
        "background-color: #f0f0f0; "
        "color: black; }"
        );

    QStringList headers;
    headers << "" << "Nombre de Campo" << "Tipo de Dato"; // PK vacío
    tablaCampos->setHorizontalHeaderLabels(headers);

    // Columna PK con ancho fijo
    tablaCampos->setColumnWidth(0, 50);

    // Insertar celda PK vacía (sin checkbox)
    QTableWidgetItem *pkItem = new QTableWidgetItem();
    pkItem->setFlags(pkItem->flags() & ~Qt::ItemIsEditable); // No editable
    pkItem->setTextAlignment(Qt::AlignCenter);
    tablaCampos->setItem(0, 0, pkItem);

    // Columna Field Name (editable QTableWidgetItem)
    QTableWidgetItem *nombreItem = new QTableWidgetItem();
    nombreItem->setText("Nuevo Campo");
    tablaCampos->setItem(0, 1, nombreItem);

    // Columna Data Type (QComboBox en la celda)
    QComboBox *tipoCombo = new QComboBox();
    tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
    connect(tipoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TablaCentralWidget::actualizarPropiedades);
    tablaCampos->setCellWidget(0, 2, tipoCombo);

    tablaCampos->horizontalHeader()->setStretchLastSection(true);
}

void TablaCentralWidget::configurarTablaPropiedades() {
    QStringList headers;
    headers << "Propiedad" << "Valor";
    tablaPropiedades->setHorizontalHeaderLabels(headers);
    tablaPropiedades->horizontalHeader()->setStretchLastSection(true);
    tablaPropiedades->verticalHeader()->setVisible(false);
}

void TablaCentralWidget::agregarCampo() {
    int row = tablaCampos->rowCount();
    tablaCampos->insertRow(row);

    // Columna PK (vacía, sin checkbox)
    QTableWidgetItem *pkItem = new QTableWidgetItem();
    pkItem->setFlags(pkItem->flags() & ~Qt::ItemIsEditable); // No editable
    pkItem->setTextAlignment(Qt::AlignCenter);
    tablaCampos->setItem(row, 0, pkItem);

    // Columna Field Name (editable QTableWidgetItem)
    QTableWidgetItem *nombreItem = new QTableWidgetItem("Nuevo Campo");
    tablaCampos->setItem(row, 1, nombreItem);

    // Columna Data Type (QComboBox en la celda)
    QComboBox *tipoCombo = new QComboBox();
    tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
    connect(tipoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &TablaCentralWidget::actualizarPropiedades);
    tablaCampos->setCellWidget(row, 2, tipoCombo);
}

void TablaCentralWidget::actualizarPropiedades() {
    tablaPropiedades->clearContents();
    tablaPropiedades->setRowCount(0);

    int currentRow = tablaCampos->currentRow();
    if (currentRow == -1) return;

    QComboBox *tipoCombo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(currentRow, 2));
    if (!tipoCombo) return;

    QString tipoDato = tipoCombo->currentText();

    if (tipoDato == "TEXTO") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Tamaño de campo"));

        QSpinBox *spinBox = new QSpinBox();
        spinBox->setRange(1, 255);
        spinBox->setValue(50);
        tablaPropiedades->setCellWidget(0, 1, spinBox);
    }
    else if (tipoDato == "NUMERO") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Tipo de número"));

        QComboBox *numeroCombo = new QComboBox();
        numeroCombo->addItems({"Entero", "Decimal", "Doble", "Byte"});
        tablaPropiedades->setCellWidget(0, 1, numeroCombo);
    }
    else if (tipoDato == "MONEDA") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Formato"));

        QComboBox *monedaCombo = new QComboBox();
        monedaCombo->addItems({"Moneda Lps", "Dollar", "Euros", "Millares"});
        tablaPropiedades->setCellWidget(0, 1, monedaCombo);
    }
    else if (tipoDato == "FECHA") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Formato"));

        QComboBox *fechaCombo = new QComboBox();
        fechaCombo->addItems({"DD-MM-YY", "DD/MM/YY", "DD/MESTEXTO/YYYY"});
        tablaPropiedades->setCellWidget(0, 1, fechaCombo);
    }
}

// Método público para establecer PK desde otra clase
void TablaCentralWidget::establecerPK() {
    int currentRow = tablaCampos->currentRow();
    if (currentRow == -1) {
        QMessageBox::information(this, "Selección requerida", "Por favor, seleccione una fila primero.");
        return;
    }

    // Quitar PK de cualquier otra fila
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaCampos->item(row, 0);
        if (pkItem && pkItem->text() == "🔑") {
            pkItem->setText("");
            pkItem->setToolTip("");
        }
    }

    // Establecer PK en la fila seleccionada
    QTableWidgetItem *pkItem = tablaCampos->item(currentRow, 0);
    if (pkItem) {
        pkItem->setText("🔑");
        pkItem->setToolTip("Llave Primaria");
    }
}

// Método para obtener la fila que actualmente es PK
int TablaCentralWidget::obtenerFilaPK() const {
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaCampos->item(row, 0);
        if (pkItem && pkItem->text() == "🔑") {
            return row;
        }
    }
    return -1; // No hay PK
}

// Método para obtener el nombre del campo PK
QString TablaCentralWidget::obtenerNombrePK() const {
    int pkRow = obtenerFilaPK();
    if (pkRow != -1) {
        QTableWidgetItem *nombreItem = tablaCampos->item(pkRow, 1);
        if (nombreItem) {
            return nombreItem->text();
        }
    }
    return "";
}

QString TablaCentralWidget::obtenerPropiedadesCampo(int row) const {
    if (row < 0 || row >= tablaCampos->rowCount()) return "";

    QComboBox *tipoCombo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(row, 2));
    if (!tipoCombo) return "";

    QString tipoDato = tipoCombo->currentText();
    QString propiedades;

    if (tipoDato == "TEXTO") {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(tablaPropiedades->cellWidget(0, 1));
        if (spinBox) propiedades = QString("Tamaño: %1").arg(spinBox->value());
    }
    else if (tipoDato == "NUMERO") {
        QComboBox *numeroCombo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (numeroCombo) propiedades = QString("Tipo: %1").arg(numeroCombo->currentText());
    }
    else if (tipoDato == "MONEDA") {
        QComboBox *monedaCombo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (monedaCombo) propiedades = QString("Formato: %1").arg(monedaCombo->currentText());
    }
    else if (tipoDato == "FECHA") {
        QComboBox *fechaCombo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (fechaCombo) propiedades = QString("Formato: %1").arg(fechaCombo->currentText());
    }

    return propiedades;
}

// 🔹 Exportar todos los campos como QVector<Campo>
QVector<Campo> TablaCentralWidget::obtenerCampos() const {
    QVector<Campo> campos;
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        Campo c;
        if (tablaCampos->item(row, 1)) {
            c.nombre = tablaCampos->item(row, 1)->text();
        }
        QComboBox *combo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(row, 2));
        if (combo) {
            c.tipo = combo->currentText();
        }
        campos.append(c);
    }
    return campos;
}

// 🔹 Cargar campos desde Metadata
void TablaCentralWidget::cargarCampos(const QVector<Campo>& campos) {
    tablaCampos->setRowCount(0); // limpiar

    for (const Campo& c : campos) {
        int row = tablaCampos->rowCount();
        tablaCampos->insertRow(row);

        // Columna PK (vacía por ahora)
        QTableWidgetItem *pkItem = new QTableWidgetItem();
        pkItem->setFlags(pkItem->flags() & ~Qt::ItemIsEditable);
        pkItem->setTextAlignment(Qt::AlignCenter);
        tablaCampos->setItem(row, 0, pkItem);

        // Nombre del campo
        QTableWidgetItem *nombreItem = new QTableWidgetItem(c.nombre);
        tablaCampos->setItem(row, 1, nombreItem);

        // Tipo de dato
        QComboBox *tipoCombo = new QComboBox();
        tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
        int index = tipoCombo->findText(c.tipo);
        if (index != -1) tipoCombo->setCurrentIndex(index);
        tablaCampos->setCellWidget(row, 2, tipoCombo);
    }
}
