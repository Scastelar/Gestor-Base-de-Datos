#include "tablacentralwidget.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>

TablaCentralWidget::TablaCentralWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(this);

    // Crear tabla
    tablaCampos = new QTableWidget(1, 3, this); // Cambiado a 3 columnas: PK, Nombre, Tipo
    configurarTablaCampos();
    QStringList headers = {"PK", "Nombre del Campo", "Tipo de Dato"};
    tablaCampos->setHorizontalHeaderLabels(headers);

    // Ajustes de estilo
    tablaCampos->horizontalHeader()->setStretchLastSection(true);
    tablaCampos->verticalHeader()->setVisible(false);
    tablaCampos->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaCampos->setSelectionMode(QAbstractItemView::SingleSelection);
    tablaCampos->setAlternatingRowColors(true);

    // Conectar señal de cambio de selección
    connect(tablaCampos, &QTableWidget::itemSelectionChanged, this, &TablaCentralWidget::actualizarPropiedades);

    tablaPropiedades = new QTableWidget(0, 2, this);
    configurarTablaPropiedades();

    layoutPrincipal->addWidget(tablaCampos);
    layoutPrincipal->addWidget(tablaPropiedades);
}

void TablaCentralWidget::configurarTablaCampos() {
    tablaCampos->setStyleSheet(
        "QTableWidget::item:selected { "
        "background-color: #f0f0f0; "
        "color: black; }"
        );

    // Configurar anchos de columnas
    tablaCampos->setColumnWidth(0, 40);  // Columna PK más estrecha
    tablaCampos->setColumnWidth(1, 150); // Columna Nombre
    tablaCampos->setColumnWidth(2, 120); // Columna Tipo

    // Insertar primera fila con PK activada por defecto
    tablaCampos->setRowCount(1);

    // Columna PK (Texto "🔑" para indicar PK)
    QTableWidgetItem *pkItem = new QTableWidgetItem("🔑");
    pkItem->setFlags(pkItem->flags() & ~Qt::ItemIsEditable); // No editable
    pkItem->setTextAlignment(Qt::AlignCenter);
    pkItem->setToolTip("Llave Primaria");
    tablaCampos->setItem(0, 0, pkItem);

    // Columna Field Name (editable QTableWidgetItem)
    QTableWidgetItem *nombreItem = new QTableWidgetItem("ID");
    tablaCampos->setItem(0, 1, nombreItem);

    // Columna Data Type (QComboBox en la celda)
    QComboBox *tipoCombo = new QComboBox();
    tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
    tipoCombo->setCurrentText("NUMERO"); // Tipo por defecto para PK
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
    tablaPropiedades->setColumnWidth(0, 150);
}

void TablaCentralWidget::agregarCampo() {
    int row = tablaCampos->rowCount();
    tablaCampos->insertRow(row);

    // Columna PK (Vacía por defecto para nuevos campos)
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

void TablaCentralWidget::eliminarCampo() {
    int currentRow = tablaCampos->currentRow();
    if (currentRow == -1) {
        QMessageBox::information(this, "Selección requerida", "Por favor, seleccione un campo para eliminar.");
        return;
    }

    // Verificar si es el campo PK
    QTableWidgetItem *pkItem = tablaCampos->item(currentRow, 0);
    if (pkItem && pkItem->text() == "🔑") {
        QMessageBox::warning(this, "Error", "No se puede eliminar el campo de clave primaria.");
        return;
    }

    // Confirmar eliminación
    QTableWidgetItem *nombreItem = tablaCampos->item(currentRow, 1);
    QString nombreCampo = nombreItem ? nombreItem->text() : "Campo seleccionado";

    QMessageBox::StandardButton respuesta = QMessageBox::question(
        this,
        "Confirmar eliminación",
        QString("¿Está seguro de que desea eliminar el campo '%1'?").arg(nombreCampo),
        QMessageBox::Yes | QMessageBox::No
        );

    if (respuesta == QMessageBox::Yes) {
        tablaCampos->removeRow(currentRow);
    }
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
        monedaCombo->addItems({"Moneda Lps", "Dólar", "Euros", "Millares"});
        tablaPropiedades->setCellWidget(0, 1, monedaCombo);
    }
    else if (tipoDato == "FECHA") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Formato"));

        QComboBox *fechaCombo = new QComboBox();
        fechaCombo->addItems({"DD-MM-YY", "DD/MM/YY", "DD/MES/YYYY", "YYYY-MM-DD"});
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

// 🔹 Exportar todos los campos como QVector<Campo>
QVector<Campo> TablaCentralWidget::obtenerCampos() const {
    QVector<Campo> campos;
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        Campo c;

        // Obtener nombre
        if (tablaCampos->item(row, 1)) {
            c.nombre = tablaCampos->item(row, 1)->text();
        }

        // Obtener tipo
        QComboBox *combo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(row, 2));
        if (combo) {
            c.tipo = combo->currentText();
        }

        // Obtener si es PK (basado en el texto "🔑")
        QTableWidgetItem *pkItem = tablaCampos->item(row, 0);
        if (pkItem) {
            c.esPK = (pkItem->text() == "🔑");
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

        // Columna PK (Texto "🔑" si es PK, vacío si no)
        QTableWidgetItem *pkItem = new QTableWidgetItem();
        pkItem->setFlags(pkItem->flags() & ~Qt::ItemIsEditable); // No editable
        pkItem->setTextAlignment(Qt::AlignCenter);
        if (c.esPK) {
            pkItem->setText("🔑");
            pkItem->setToolTip("Llave Primaria");
        }
        tablaCampos->setItem(row, 0, pkItem);

        // Nombre del campo
        QTableWidgetItem *nombreItem = new QTableWidgetItem(c.nombre);
        tablaCampos->setItem(row, 1, nombreItem);

        // Tipo de dato
        QComboBox *tipoCombo = new QComboBox();
        tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
        int index = tipoCombo->findText(c.tipo);
        if (index != -1) tipoCombo->setCurrentIndex(index);
        connect(tipoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &TablaCentralWidget::actualizarPropiedades);
        tablaCampos->setCellWidget(row, 2, tipoCombo);
    }

    // Forzar actualización de propiedades para la primera fila
    if (tablaCampos->rowCount() > 0) {
        tablaCampos->setCurrentCell(0, 1);
        actualizarPropiedades();
    }
}

// Método para validar que existe exactamente una PK
bool TablaCentralWidget::validarPK() const {
    int countPK = 0;
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaCampos->item(row, 0);
        if (pkItem && pkItem->text() == "🔑") {
            countPK++;
        }
    }
    return countPK == 1;
}
