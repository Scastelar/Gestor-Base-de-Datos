#include "vistadiseno.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QComboBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QFile>
#include <QTextStream>
#include <QTimer>
#include <QDir>

VistaDiseno::VistaDiseno(QWidget *parent)
    : QWidget(parent), guardandoMetadatos(false),bloqueandoEdicion(false)
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
    connect(tablaCampos, &QTableWidget::itemSelectionChanged, this, &VistaDiseno::actualizarPropiedades);

    connect(tablaCampos, &QTableWidget::currentCellChanged,
            this, &VistaDiseno::on_tablaCampos_currentCellChanged);
    connect(tablaCampos, &QTableWidget::cellChanged,
            this, &VistaDiseno::on_tablaCampos_cellChanged);
    connect(tablaCampos, &QTableWidget::itemChanged, this, &VistaDiseno::on_campoEditado);

    tablaPropiedades = new QTableWidget(0, 2, this);
    configurarTablaPropiedades();

    layoutPrincipal->addWidget(tablaCampos);
    layoutPrincipal->addWidget(tablaPropiedades);
}

void VistaDiseno::configurarTablaCampos() {
    tablaCampos->setStyleSheet(
        "QTableWidget::item:selected { "
        "background-color: #f0f0f0; "
        "color: black; }"
        "QTableWidget::item[readonly=\"true\"] { "
        "background-color: #f5f5f5; "
        "color: #888888; }"
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
            this, &VistaDiseno::actualizarPropiedades);
    tablaCampos->setCellWidget(0, 2, tipoCombo);

    tablaCampos->horizontalHeader()->setStretchLastSection(true);

     nombresAnteriores.clear();
}

void VistaDiseno::configurarTablaPropiedades() {
    QStringList headers;
    headers << "Propiedad" << "Valor";
    tablaPropiedades->setHorizontalHeaderLabels(headers);
    tablaPropiedades->horizontalHeader()->setStretchLastSection(true);
    tablaPropiedades->verticalHeader()->setVisible(false);
    tablaPropiedades->setColumnWidth(0, 150);
}

void VistaDiseno::agregarCampo() {
    int row = tablaCampos->rowCount();
    tablaCampos->insertRow(row);

    // Columna PK
    QTableWidgetItem *pkItem = new QTableWidgetItem();
    pkItem->setFlags(pkItem->flags() & ~Qt::ItemIsEditable);
    pkItem->setTextAlignment(Qt::AlignCenter);
    tablaCampos->setItem(row, 0, pkItem);

    // Columna Field Name
    QTableWidgetItem *nombreItem = new QTableWidgetItem("Nuevo Campo");
    tablaCampos->setItem(row, 1, nombreItem);

    // 🔹 ALMACENAR NOMBRE INICIAL
    nombresAnteriores[row] = "Nuevo Campo";

    // Columna Data Type
    QComboBox *tipoCombo = new QComboBox();
    tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
    connect(tipoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &VistaDiseno::actualizarPropiedades);
    tablaCampos->setCellWidget(row, 2, tipoCombo);

    guardarMetadatos();
}

void VistaDiseno::eliminarCampo() {
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

    // Verificar si el campo está relacionado
    QTableWidgetItem *nombreItem = tablaCampos->item(currentRow, 1);
    if (nombreItem && esCampoRelacionado(nombreItem->text())) {
        QMessageBox::warning(this, "Campo relacionado",
                             "No se puede eliminar campos relacionados a otras tablas.");
        return;
    }

    // Confirmar eliminación
    QString nombreCampo = nombreItem ? nombreItem->text() : "Campo seleccionado";
    QMessageBox::StandardButton respuesta = QMessageBox::question(
        this,
        "Confirmar eliminación",
        QString("¿Está seguro de que desea eliminar el campo '%1'?").arg(nombreCampo),
        QMessageBox::Yes | QMessageBox::No
        );

    if (respuesta == QMessageBox::Yes) {
        nombresAnteriores.remove(currentRow);
        tablaCampos->removeRow(currentRow);
        guardarMetadatos();
    }
}


// Método público para establecer PK desde otra clase
void VistaDiseno::establecerPK() {
    int currentRow = tablaCampos->currentRow();
    if (currentRow == -1) {
        QMessageBox::information(this, "Selección requerida", "Por favor, seleccione una fila primero.");
        return;
    }

    // Verificar si el campo está relacionado
    QTableWidgetItem *nombreItem = tablaCampos->item(currentRow, 1);
    if (nombreItem && esCampoRelacionado(nombreItem->text())) {
        QMessageBox::warning(this, "Campo relacionado",
                             "No se puede modificar la PK de campos relacionados a otras tablas.");
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
    //QTimer::singleShot(100, this, &VistaDiseno::guardarMetadatos);
    guardarMetadatos();

}

// Método para obtener la fila que actualmente es PK
int VistaDiseno::obtenerFilaPK() const {
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaCampos->item(row, 0);
        if (pkItem && pkItem->text() == "🔑") {
            return row;
        }
    }
    return -1; // No hay PK
}

// Método para obtener el nombre del campo PK
QString VistaDiseno::obtenerNombrePK() const {
    int pkRow = obtenerFilaPK();
    if (pkRow != -1) {
        QTableWidgetItem *nombreItem = tablaCampos->item(pkRow, 1);
        if (nombreItem) {
            return nombreItem->text();
        }
    }
    return "";
}

// 🔹 Exportar todos los campos como QVector<Campo> con propiedades
QVector<Campo> VistaDiseno::obtenerCampos() const {
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

        // Obtener propiedad según el tipo
        if (c.tipo == "TEXTO") {
            // Buscar en tabla de propiedades
            if (propiedadesPorFila.contains(row)) {
                c.propiedad = propiedadesPorFila.value(row);
            } else {
                c.propiedad = 255; // Valor por defecto
            }
        }
        else if (c.tipo == "NUMERO") {
            if (propiedadesPorFila.contains(row)) {
                c.propiedad = propiedadesPorFila.value(row);
            } else {
                c.propiedad = "entero"; // Valor por defecto
            }
        }
        else if (c.tipo == "MONEDA") {
            if (propiedadesPorFila.contains(row)) {
                c.propiedad = propiedadesPorFila.value(row);
            } else {
                c.propiedad = "Lempira"; // Valor por defecto
            }
        }
        else if (c.tipo == "FECHA") {
            if (propiedadesPorFila.contains(row)) {
                c.propiedad = propiedadesPorFila.value(row);
            } else {
                c.propiedad = "DD-MM-YY"; // Valor por defecto
            }
        }

        campos.append(c);
    }
    return campos;
}

// 🔹 Cargar campos desde Metadata con propiedades
void VistaDiseno::cargarCampos(const QVector<Campo>& campos) {
    tablaCampos->setRowCount(0); // limpiar
    propiedadesPorFila.clear();
    nombresAnteriores.clear(); // 🔹 Limpiar nombres anteriores

    for (int i = 0; i < campos.size(); i++) {
        const Campo& c = campos[i];
        int row = tablaCampos->rowCount();
        tablaCampos->insertRow(row);

        // Columna PK
        QTableWidgetItem *pkItem = new QTableWidgetItem();
        pkItem->setFlags(pkItem->flags() & ~Qt::ItemIsEditable);
        pkItem->setTextAlignment(Qt::AlignCenter);
        if (c.esPK) {
            pkItem->setText("🔑");
            pkItem->setToolTip("Llave Primaria");
        }
        tablaCampos->setItem(row, 0, pkItem);

        // Nombre del campo
        QTableWidgetItem *nombreItem = new QTableWidgetItem(c.nombre);
        tablaCampos->setItem(row, 1, nombreItem);

        // 🔹 ALMACENAR NOMBRE INICIAL
        nombresAnteriores[row] = c.nombre;

        // Tipo de dato
        QComboBox *tipoCombo = new QComboBox();
        tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
        int index = tipoCombo->findText(c.tipo);
        if (index != -1) tipoCombo->setCurrentIndex(index);
        connect(tipoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, &VistaDiseno::actualizarPropiedades);
        tablaCampos->setCellWidget(row, 2, tipoCombo);

        // Guardar propiedad en el mapa
        if (c.propiedad.isValid()) {
            propiedadesPorFila[row] = c.propiedad;
        } else {
            // Establecer propiedad por defecto según el tipo
            if (c.tipo == "TEXTO") {
                propiedadesPorFila[row] = 255;
            } else if (c.tipo == "NUMERO") {
                propiedadesPorFila[row] = "entero";
            } else if (c.tipo == "MONEDA") {
                propiedadesPorFila[row] = "Lempira";
            } else if (c.tipo == "FECHA") {
                propiedadesPorFila[row] = "DD-MM-YY";
            }
        }
    }

    // Forzar actualización de propiedades para la primera fila
    if (tablaCampos->rowCount() > 0) {
        tablaCampos->setCurrentCell(0, 1);
        actualizarPropiedades();
    }
}

// 🔹 Actualizar propiedades cuando cambia el tipo de dato
void VistaDiseno::actualizarPropiedades() {
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

        // Cargar valor guardado o usar valor por defecto
        int valor = propiedadesPorFila.value(currentRow, 255).toInt();
        spinBox->setValue(valor);

        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                [this, currentRow](int value) {
                    propiedadesPorFila[currentRow] = value;
                });

        tablaPropiedades->setCellWidget(0, 1, spinBox);
    }
    else if (tipoDato == "NUMERO") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Tipo de número"));

        QComboBox *numeroCombo = new QComboBox();
        numeroCombo->addItems({"entero", "decimal", "doble", "byte"});

        // Cargar valor guardado o usar valor por defecto
        QString valor = propiedadesPorFila.value(currentRow, "entero").toString();
        int index = numeroCombo->findText(valor);
        if (index != -1) numeroCombo->setCurrentIndex(index);

        connect(numeroCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, currentRow, numeroCombo](int index) {
                    propiedadesPorFila[currentRow] = numeroCombo->currentText();
                });

        tablaPropiedades->setCellWidget(0, 1, numeroCombo);
    }
    else if (tipoDato == "MONEDA") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Formato"));

        QComboBox *monedaCombo = new QComboBox();
        monedaCombo->addItems({"Lempira", "Dólar", "Euros", "Millares"});

        // Cargar valor guardado o usar valor por defecto
        QString valor = propiedadesPorFila.value(currentRow, "Lempira").toString();
        int index = monedaCombo->findText(valor);
        if (index != -1) monedaCombo->setCurrentIndex(index);

        connect(monedaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, currentRow, monedaCombo](int index) {
                    propiedadesPorFila[currentRow] = monedaCombo->currentText();
                });

        tablaPropiedades->setCellWidget(0, 1, monedaCombo);
    }
    else if (tipoDato == "FECHA") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Formato"));

        QComboBox *fechaCombo = new QComboBox();
        fechaCombo->addItems({"DD-MM-YY", "DD/MM/YY", "DD/MES/YYYY", "YYYY-MM-DD"});

        // Cargar valor guardado o usar valor por defecto
        QString valor = propiedadesPorFila.value(currentRow, "DD-MM-YY").toString();
        int index = fechaCombo->findText(valor);
        if (index != -1) fechaCombo->setCurrentIndex(index);

        connect(fechaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, currentRow, fechaCombo](int index) {
                    propiedadesPorFila[currentRow] = fechaCombo->currentText();
                });

        tablaPropiedades->setCellWidget(0, 1, fechaCombo);
    }
}

// 🔹 Guardar propiedad actual cuando cambia de fila
void VistaDiseno::on_tablaCampos_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn) {
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousColumn);

    // Guardar propiedad de la fila anterior
    if (previousRow != -1) {
        guardarPropiedadFila(previousRow);
    }

    // Actualizar propiedades para la nueva fila seleccionada
    if (currentRow != -1) {
        actualizarPropiedades();
    }
}

// 🔹 Método auxiliar para guardar propiedad de una fila específica
void VistaDiseno::guardarPropiedadFila(int row) {
    if (tablaPropiedades->rowCount() == 0) return;

    QComboBox *tipoCombo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(row, 2));
    if (!tipoCombo) return;

    QString tipoDato = tipoCombo->currentText();

    if (tipoDato == "TEXTO") {
        QSpinBox *spinBox = qobject_cast<QSpinBox*>(tablaPropiedades->cellWidget(0, 1));
        if (spinBox) {
            propiedadesPorFila[row] = spinBox->value();
        }
    }
    else if (tipoDato == "NUMERO") {
        QComboBox *combo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (combo) {
            propiedadesPorFila[row] = combo->currentText();
        }
    }
    else if (tipoDato == "MONEDA") {
        QComboBox *combo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (combo) {
            propiedadesPorFila[row] = combo->currentText();
        }
    }
    else if (tipoDato == "FECHA") {
        QComboBox *combo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (combo) {
            propiedadesPorFila[row] = combo->currentText();
        }
    }
}

// 🔹 Limpiar propiedades cuando se elimina una fila
void VistaDiseno::on_tablaCampos_cellChanged(int row, int column) {
    // Si se elimina una fila, remover su propiedad
    if (column == 0 && row >= tablaCampos->rowCount()) {
        propiedadesPorFila.remove(row);
    }
}

// Método para validar que existe exactamente una PK
bool VistaDiseno::validarPK() const {
    int countPK = 0;
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaCampos->item(row, 0);
        if (pkItem && pkItem->text() == "🔑") {
            countPK++;
        }
    }
    return countPK == 1;
}

void VistaDiseno::setCamposRelacionados(const QSet<QString>& camposRelacionados) {
    this->camposRelacionados = camposRelacionados;
    actualizarEstadoCampos();
}

void VistaDiseno::setNombreTabla(const QString& nombre) {
    this->nombreTablaActual = nombre;
}

bool VistaDiseno::esCampoRelacionado(const QString& nombreCampo) const {
    return camposRelacionados.contains(nombreCampo);
}

void VistaDiseno::actualizarEstadoCampos() {
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        QTableWidgetItem *nombreItem = tablaCampos->item(row, 1);
        if (nombreItem) {
            QString nombreCampo = nombreItem->text();
            bool esRelacionado = esCampoRelacionado(nombreCampo);

            if (nombresAnteriores.contains(row) && nombresAnteriores[row] != nombreCampo) {
                nombresAnteriores[row] = nombreCampo;
            }

            // Hacer el campo de solo lectura si está relacionado
            nombreItem->setFlags(esRelacionado ?
                                     nombreItem->flags() & ~Qt::ItemIsEditable :
                                     nombreItem->flags() | Qt::ItemIsEditable);

            QComboBox *tipoCombo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(row, 2));
            if (tipoCombo) {
                tipoCombo->setEnabled(!esRelacionado);

                // 🔹 Si está relacionado, forzar el tipo original
                if (esRelacionado) {
                    // Podrías almacenar también el tipo original si es necesario
                    tipoCombo->setToolTip("Tipo bloqueado por relación");
                } else {
                    tipoCombo->setToolTip("");
                }
            }
        }
    }
}

void VistaDiseno::on_campoEditado(QTableWidgetItem *item) {
    if (bloqueandoEdicion) return;

    if (item->column() == 1) { // Columna de nombre
        int row = item->row();
        QString nombreAnterior;

        // 🔹 Obtener el nombre anterior del campo (antes del cambio)
        // Necesitamos almacenar los nombres anteriores
        if (nombresAnteriores.contains(row)) {
            nombreAnterior = nombresAnteriores[row];
        }

        QString nuevoNombre = item->text();

        // 🔹 Verificar si el campo YA ESTABA RELACIONADO (nombre anterior)
        if (!nombreAnterior.isEmpty() && esCampoRelacionado(nombreAnterior)) {
            QMessageBox::warning(this, "Campo relacionado",
                                 "No se puede modificar campos relacionados a otras tablas.");

            // Revertir al nombre anterior
            bloqueandoEdicion = true;
            item->setText(nombreAnterior);
            bloqueandoEdicion = false;
            return;
        }

        // 🔹 También verificar si el nuevo nombre ya está relacionado
        if (esCampoRelacionado(nuevoNombre)) {
            QMessageBox::warning(this, "Campo relacionado",
                                 "No se puede usar un nombre que ya está relacionado en otra tabla.");

            // Revertir al nombre anterior si existe, o dejar vacío
            bloqueandoEdicion = true;
            item->setText(nombreAnterior.isEmpty() ? "" : nombreAnterior);
            bloqueandoEdicion = false;
            return;
        }

        // 🔹 Actualizar el nombre anterior para la próxima vez
        nombresAnteriores[row] = nuevoNombre;
    }

    // Guardar metadatos
    guardarMetadatos();
}

void VistaDiseno::guardarMetadatos() {
    if (nombreTablaActual.isEmpty() || guardandoMetadatos) return;

    guardandoMetadatos = true;

    try {
        Metadata meta;
        meta.nombreTabla = nombreTablaActual;
        meta.campos = obtenerCampos();

        // Validar PK antes de guardar
        if (!meta.validarPK()) {
            QMessageBox::warning(this, "Error de validación",
                                 "Debe existir exactamente una clave primaria (PK).");
            guardandoMetadatos = false;
            return;
        }

        meta.guardar();
        emit metadatosModificados();

    } catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Error al guardar", e.what());
    }

    guardandoMetadatos = false;
}


