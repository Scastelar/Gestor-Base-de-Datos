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

    connect(tablaCampos, &QTableWidget::cellDoubleClicked,
            this, &VistaDiseno::onCellDoubleClicked);


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
            this, [this]() {
                actualizarPropiedades();
                guardarMetadatos();   // 🔹 Guardar al cambiar tipo
            });
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
    tablaCampos->blockSignals(true);
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
            this, [this]() {
                actualizarPropiedades();
                guardarMetadatos();   // 🔹 Guardar al cambiar tipo
            });
    tablaCampos->setCellWidget(row, 2, tipoCombo);

    guardarMetadatos();
    tablaCampos->blockSignals(false);
}

void VistaDiseno::eliminarCampo() {
    tablaCampos->blockSignals(true);
    int currentRow = tablaCampos->currentRow();
    if (currentRow == -1) {
        QMessageBox::information(this, "Selección requerida", "Por favor, seleccione un campo para eliminar.");
        tablaCampos->blockSignals(false);
        return;
    }

    // Verificar si es el campo PK
    QTableWidgetItem *pkItem = tablaCampos->item(currentRow, 0);
    if (pkItem && pkItem->text() == "🔑") {
        QMessageBox::warning(this, "Error", "No se puede eliminar el campo de clave primaria.");
        tablaCampos->blockSignals(false);
        return;
    }

    // Bloquear campos con relaciones
    QTableWidgetItem *nombreItem = tablaCampos->item(currentRow, 1);
    if (nombreItem && esCampoRelacionado(nombreItem->text())) {
        tablaCampos->blockSignals(false);
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
    tablaCampos->blockSignals(false);
}


// Método público para establecer PK desde otra clase
void VistaDiseno::establecerPK() {
    tablaCampos->blockSignals(true);
    int currentRow = tablaCampos->currentRow();
    if (currentRow == -1) {
        QMessageBox::information(this, "Selección requerida", "Por favor, seleccione una fila primero.");
        tablaCampos->blockSignals(false);
        return;
    }

    // Verificar si el campo está relacionado
    QTableWidgetItem *nombreItem = tablaCampos->item(currentRow, 1);
    if (nombreItem && esCampoRelacionado(nombreItem->text())) {
        tablaCampos->blockSignals(false);
        return;
    }

    // Verificar si hay algún campo relacionado que sea PK actualmente
    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        QTableWidgetItem *pkCheck = tablaCampos->item(row, 0);
        QTableWidgetItem *nombreCheck = tablaCampos->item(row, 1);

        if (pkCheck && pkCheck->text() == "🔑" && nombreCheck &&
            esCampoRelacionado(nombreCheck->text())) {
            tablaCampos->blockSignals(false);
            return;
        }
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
    guardarMetadatos();
    tablaCampos->blockSignals(false);
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

// Exportar todos los campos como QVector<Campo> con propiedades
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

        // OBTENER PROPIEDAD CON VALIDACIÓN
        if (propiedadesPorFila.contains(row)) {
            c.propiedad = propiedadesPorFila.value(row);
        } else {
            // Establecer valor por defecto si no existe
            if (c.tipo == "TEXTO") {
                c.propiedad = 255;
            } else if (c.tipo == "NUMERO") {
                c.propiedad = "entero";
            } else if (c.tipo == "MONEDA") {
                c.propiedad = "Lempira";
            } else if (c.tipo == "FECHA") {
                c.propiedad = "DD-MM-YY";
            }
        }

        campos.append(c);
    }
    return campos;
}

// 🔹 Cargar campos desde Metadata con propiedades
void VistaDiseno::cargarCampos(const QVector<Campo>& campos) {
    tablaCampos->blockSignals(true);
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

         // Almacenar nombre inicial
        nombresAnteriores[row] = c.nombre;

        // Tipo de dato
        QComboBox *tipoCombo = new QComboBox();
        tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
        int index = tipoCombo->findText(c.tipo);
        if (index != -1) tipoCombo->setCurrentIndex(index);
        connect(tipoCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                this, [this]() {
                    actualizarPropiedades();
                    guardarMetadatos();   // 🔹 Guardar al cambiar tipo
                });
        tablaCampos->setCellWidget(row, 2, tipoCombo);

        // ⭐ VALIDAR PROPIEDAD SEGÚN EL TIPO ANTES DE GUARDAR
                QVariant propiedadCorrecta;
        if (c.propiedad.isValid()) {
            // Verificar que la propiedad coincida con el tipo
            if (c.tipo == "TEXTO") {
                bool ok;
                int valorTexto = c.propiedad.toInt(&ok);
                propiedadCorrecta = ok && valorTexto > 0 ? valorTexto : 255;
            } else if (c.tipo == "NUMERO") {
                QString valorNumero = c.propiedad.toString();
                if (valorNumero == "entero" || valorNumero == "decimal" ||
                    valorNumero == "doble" || valorNumero == "byte") {
                    propiedadCorrecta = valorNumero;
                } else {
                    propiedadCorrecta = "entero"; // Corregir valor inválido
                }
            } else if (c.tipo == "MONEDA") {
                QString valorMoneda = c.propiedad.toString();
                if (valorMoneda == "Lempira" || valorMoneda == "Dólar" ||
                    valorMoneda == "Euros" || valorMoneda == "Millares") {
                    propiedadCorrecta = valorMoneda;
                } else {
                    propiedadCorrecta = "Lempira"; // Corregir valor inválido
                }
            } else if (c.tipo == "FECHA") {
                QString valorFecha = c.propiedad.toString();
                if (valorFecha == "DD-MM-YY" || valorFecha == "DD/MM/YY" ||
                    valorFecha == "DD/MES/YYYY" || valorFecha == "YYYY-MM-DD") {
                    propiedadCorrecta = valorFecha;
                } else {
                    propiedadCorrecta = "DD-MM-YY"; // Corregir valor inválido
                }
            }
        } else {
            // Establecer propiedad por defecto según el tipo
            if (c.tipo == "TEXTO") {
                propiedadCorrecta = 255;
            } else if (c.tipo == "NUMERO") {
                propiedadCorrecta = "entero";
            } else if (c.tipo == "MONEDA") {
                propiedadCorrecta = "Lempira";
            } else if (c.tipo == "FECHA") {
                propiedadCorrecta = "DD-MM-YY";
            }
        }
        propiedadesPorFila[row] = propiedadCorrecta;
    }

    // ⭐ CRÍTICO: Actualizar estado de campos relacionados DESPUÉS de cargar
    QTimer::singleShot(100, this, [this]() {
        actualizarEstadoCampos();
        qDebug() << "🔒 Campos relacionados actualizados:" << camposRelacionados;
    });

    // Forzar actualización de propiedades para la primera fila
    if (tablaCampos->rowCount() > 0) {
        tablaCampos->setCurrentCell(0, 1);
        actualizarPropiedades();
    }
    tablaCampos->blockSignals(false);
}

// 🔹 Actualizar propiedades cuando cambia el tipo de dato
void VistaDiseno::actualizarPropiedades() {
    tablaPropiedades->clearContents();
    tablaPropiedades->setRowCount(0);

    int currentRow = tablaCampos->currentRow();
    if (currentRow == -1) return;

    // ⭐ NUEVA VALIDACIÓN: Verificar si el campo está relacionado
    QTableWidgetItem *nombreItem = tablaCampos->item(currentRow, 1);
    bool esRelacionado = nombreItem && esCampoRelacionado(nombreItem->text());

    QComboBox *tipoCombo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(currentRow, 2));
    if (!tipoCombo) return;

    QString tipoDato = tipoCombo->currentText();

    // Limpiar propiedad anterior si el tipo cambió
    QString tipoAnterior = "";
    if (propiedadesPorFila.contains(currentRow)) {
        QVariant propAnterior = propiedadesPorFila[currentRow];

        if (propAnterior.typeId() == QMetaType::Int) {
            tipoAnterior = "TEXTO";
        } else if (propAnterior.toString() == "entero" || propAnterior.toString() == "decimal" ||
                   propAnterior.toString() == "doble" || propAnterior.toString() == "byte") {
            tipoAnterior = "NUMERO";
        } else if (propAnterior.toString() == "Lempira" || propAnterior.toString() == "Dólar" ||
                   propAnterior.toString() == "Euros" || propAnterior.toString() == "Millares") {
            tipoAnterior = "MONEDA";
        } else if (propAnterior.toString().contains("-") || propAnterior.toString().contains("/")) {
            tipoAnterior = "FECHA";
        }
    }

    // Si cambió el tipo, establecer valor por defecto
    if (tipoAnterior != "" && tipoAnterior != tipoDato) {
        qDebug() << "⚠️ Tipo cambió de" << tipoAnterior << "a" << tipoDato << "- Estableciendo valor por defecto";

        if (tipoDato == "TEXTO") {
            propiedadesPorFila[currentRow] = 255;
        } else if (tipoDato == "NUMERO") {
            propiedadesPorFila[currentRow] = "entero";
        } else if (tipoDato == "MONEDA") {
            propiedadesPorFila[currentRow] = "Lempira";
        } else if (tipoDato == "FECHA") {
            propiedadesPorFila[currentRow] = "DD-MM-YY";
        }
    }

    if (tipoDato == "TEXTO") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Tamaño de campo"));

        QSpinBox *spinBox = new QSpinBox();
        spinBox->setRange(1, 255);
        spinBox->setEnabled(!esRelacionado); // ⭐ DESHABILITAR SI ESTÁ RELACIONADO

        // Cargar valor guardado o usar valor por defecto
        int valor = propiedadesPorFila.value(currentRow, 255).toInt();
        spinBox->setValue(valor);

        if (esRelacionado) {
            spinBox->setStyleSheet("QSpinBox { background-color: #f5f5f5; color: #888888; }");
            spinBox->setToolTip("Propiedad bloqueada por relación");
        }

        connect(spinBox, QOverload<int>::of(&QSpinBox::valueChanged),
                [this, currentRow](int value) {
                    propiedadesPorFila[currentRow] = value;
                    guardarMetadatos();
                });

        tablaPropiedades->setCellWidget(0, 1, spinBox);
    }
    else if (tipoDato == "NUMERO") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Tipo de número"));

        QComboBox *numeroCombo = new QComboBox();
        numeroCombo->addItems({"entero", "decimal", "doble", "byte"});
        numeroCombo->setEnabled(!esRelacionado);

        // Cargar valor guardado o usar valor por defecto
        QString valor = propiedadesPorFila.value(currentRow, "entero").toString();
        int index = numeroCombo->findText(valor);
        if (index != -1) numeroCombo->setCurrentIndex(index);

        if (esRelacionado) {
            numeroCombo->setStyleSheet("QComboBox { background-color: #f5f5f5; color: #888888; }");
            numeroCombo->setToolTip("Propiedad bloqueada por relación");
        }

        connect(numeroCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, currentRow, numeroCombo](int index) {
                    propiedadesPorFila[currentRow] = numeroCombo->currentText();
                    guardarMetadatos();
                });

        tablaPropiedades->setCellWidget(0, 1, numeroCombo);
    }
    else if (tipoDato == "MONEDA") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Formato"));

        QComboBox *monedaCombo = new QComboBox();
        monedaCombo->addItems({"Lempira", "Dólar", "Euros", "Millares"});
        monedaCombo->setEnabled(!esRelacionado);

        // Cargar valor guardado o usar valor por defecto
        QString valor = propiedadesPorFila.value(currentRow, "Lempira").toString();
        int index = monedaCombo->findText(valor);
        if (index != -1) monedaCombo->setCurrentIndex(index);

        if (esRelacionado) {
            monedaCombo->setStyleSheet("QComboBox { background-color: #f5f5f5; color: #888888; }");
            monedaCombo->setToolTip("Propiedad bloqueada por relación");
        }

        connect(monedaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, currentRow, monedaCombo](int index) {
                    propiedadesPorFila[currentRow] = monedaCombo->currentText();
                    guardarMetadatos();
                });

        tablaPropiedades->setCellWidget(0, 1, monedaCombo);
    }
    else if (tipoDato == "FECHA") {
        tablaPropiedades->setRowCount(1);
        tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Formato"));

        QComboBox *fechaCombo = new QComboBox();
        fechaCombo->addItems({"DD-MM-YY", "DD/MM/YY", "DD/MES/YYYY", "YYYY-MM-DD"});
        fechaCombo->setEnabled(!esRelacionado);

        // Cargar valor guardado o usar valor por defecto
        QString valor = propiedadesPorFila.value(currentRow, "DD-MM-YY").toString();
        int index = fechaCombo->findText(valor);
        if (index != -1) fechaCombo->setCurrentIndex(index);

        if (esRelacionado) {
            fechaCombo->setStyleSheet("QComboBox { background-color: #f5f5f5; color: #888888; }");
            fechaCombo->setToolTip("Propiedad bloqueada por relación");
        }

        connect(fechaCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                [this, currentRow, fechaCombo](int index) {
                    propiedadesPorFila[currentRow] = fechaCombo->currentText();
                    guardarMetadatos();
                });

        tablaPropiedades->setCellWidget(0, 1, fechaCombo);
    }
}

// Guardar propiedad actual cuando cambia de fila
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
            int valor = spinBox->value();
            propiedadesPorFila[row] = valor;
        }
    }
    else if (tipoDato == "NUMERO") {
        QComboBox *combo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (combo) {
            QString valor = combo->currentText();
            propiedadesPorFila[row] = valor;
        }
    }
    else if (tipoDato == "MONEDA") {
        QComboBox *combo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (combo) {
            QString valor = combo->currentText();
            propiedadesPorFila[row] = valor;
        }
    }
    else if (tipoDato == "FECHA") {
        QComboBox *combo = qobject_cast<QComboBox*>(tablaPropiedades->cellWidget(0, 1));
        if (combo) {
            QString valor = combo->currentText();
            propiedadesPorFila[row] = valor;
        }
    }
    // ⭐ GUARDAR AUTOMÁTICAMENTE DESPUÉS DE CAMBIAR FILA
    QTimer::singleShot(100, this, [this]() {
        guardarMetadatos();
    });
}

// 🔹 Limpiar propiedades cuando se elimina una fila
void VistaDiseno::on_tablaCampos_cellChanged(int row, int column) {

    if (bloqueandoEdicion) return;
    bloqueandoEdicion = true;

    // Si se elimina una fila, remover su propiedad
    if (column == 0 && row >= tablaCampos->rowCount()) {
        propiedadesPorFila.remove(row);
    }

    bloqueandoEdicion = false;
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

void VistaDiseno::setCamposRelacionados(const QSet<QString>& camposRel) {
    qDebug() << "🔗 Estableciendo campos relacionados:" << camposRel;
    this->camposRelacionados = camposRel;

    // Actualizar inmediatamente si ya hay campos cargados
    if (tablaCampos->rowCount() > 0) {
        actualizarEstadoCampos();
    }
}

void VistaDiseno::setNombreTabla(const QString& nombre) {
    this->nombreTablaActual = nombre;
}

bool VistaDiseno::esCampoRelacionado(const QString& nombreCampo) const {
    return camposRelacionados.contains(nombreCampo);
}

void VistaDiseno::actualizarEstadoCampos() {
    qDebug() << "🔄 Actualizando estado de" << tablaCampos->rowCount() << "campos";
    qDebug() << "🔗 Campos relacionados actuales:" << camposRelacionados;

    for (int row = 0; row < tablaCampos->rowCount(); ++row) {
        QTableWidgetItem *nombreItem = tablaCampos->item(row, 1);
        if (nombreItem) {
            QString nombreCampo = nombreItem->text();
            bool esRelacionado = esCampoRelacionado(nombreCampo);

            if (nombresAnteriores.contains(row) && nombresAnteriores[row] != nombreCampo) {
                nombresAnteriores[row] = nombreCampo;
            }

            // ⭐ BLOQUEAR COMPLETAMENTE SI ESTÁ RELACIONADO
                if (esRelacionado) {
                // Hacer el campo de solo lectura
                nombreItem->setFlags(nombreItem->flags() & ~Qt::ItemIsEditable);
                nombreItem->setBackground(QBrush(QColor(245, 245, 245))); // Gris claro
                nombreItem->setToolTip("Campo relacionado - No se puede modificar");

                // Deshabilitar combo de tipo
                QComboBox *tipoCombo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(row, 2));
                if (tipoCombo) {
                    tipoCombo->setEnabled(false);
                    tipoCombo->setStyleSheet("QComboBox { background-color: #f5f5f5; color: #888888; }");
                    tipoCombo->setToolTip("Tipo bloqueado por relación");
                }

                // Bloquear cambios de PK
                QTableWidgetItem *pkItem = tablaCampos->item(row, 0);
                if (pkItem) {
                    pkItem->setToolTip("PK bloqueada por relación - No se puede modificar");
                    pkItem->setBackground(QBrush(QColor(245, 245, 245)));
                }
            } else{
                    // Habilitar campos normales
                    nombreItem->setFlags(nombreItem->flags() | Qt::ItemIsEditable);
                    nombreItem->setBackground(QBrush(Qt::white));
                    nombreItem->setToolTip("");

                    QComboBox *tipoCombo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(row, 2));
                    if (tipoCombo) {
                        tipoCombo->setEnabled(true);
                        tipoCombo->setStyleSheet("");
                        tipoCombo->setToolTip("");
                    }

                    QTableWidgetItem *pkItem = tablaCampos->item(row, 0);
                    if (pkItem) {
                        pkItem->setToolTip(pkItem->text() == "🔑" ? "Llave Primaria" : "");
                        pkItem->setBackground(QBrush(Qt::white));
                    }
                }

            // ⭐ DESHABILITAR COMBO DE TIPO SI ESTÁ RELACIONADO
            QComboBox *tipoCombo = qobject_cast<QComboBox*>(tablaCampos->cellWidget(row, 2));
            if (tipoCombo) {
                tipoCombo->setEnabled(!esRelacionado);

                if (esRelacionado) {
                    tipoCombo->setStyleSheet("QComboBox { background-color: #f5f5f5; color: #888888; }");
                    tipoCombo->setToolTip("Tipo bloqueado por relación");
                } else {
                    tipoCombo->setStyleSheet("");
                    tipoCombo->setToolTip("");
                }
            }
        }
    }
}

void VistaDiseno::actualizarRelacionesConNuevoCampo(const QString &nombreAnterior, const QString &nuevoNombre) {
    if (nombreTablaActual.isEmpty()) return;

    qDebug() << "🔄 Actualizando relaciones: campo" << nombreAnterior << "→" << nuevoNombre;

    // Leer todas las relaciones existentes
    QFile relacionesFile("relationships.dat");
    if (!relacionesFile.open(QIODevice::ReadOnly)) {
        qDebug() << "No se pudo abrir archivo de relaciones para lectura";
        return;
    }

    QStringList relacionesActualizadas;
    QTextStream in(&relacionesFile);
    bool huboCambios = false;

    while (!in.atEnd()) {
        QString linea = in.readLine().trimmed();
        if (linea.isEmpty()) continue;

        QStringList partes = linea.split("|");
        if (partes.size() == 4) {
            QString tabla1 = partes[0];
            QString campo1 = partes[1];
            QString tabla2 = partes[2];
            QString campo2 = partes[3];

            // Actualizar si el campo pertenece a nuestra tabla
            if (tabla1 == nombreTablaActual && campo1 == nombreAnterior) {
                campo1 = nuevoNombre;
                huboCambios = true;
                qDebug() << "📝 Actualizada relación origen:" << tabla1 << "." << campo1;
            }
            if (tabla2 == nombreTablaActual && campo2 == nombreAnterior) {
                campo2 = nuevoNombre;
                huboCambios = true;
                qDebug() << "📝 Actualizada relación destino:" << tabla2 << "." << campo2;
            }

            relacionesActualizadas.append(tabla1 + "|" + campo1 + "|" + tabla2 + "|" + campo2);
        }
    }
    relacionesFile.close();

    // Reescribir el archivo si hubo cambios
    if (huboCambios) {
        if (relacionesFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            QTextStream out(&relacionesFile);
            for (const QString &relacion : relacionesActualizadas) {
                out << relacion << "\n";
            }
            relacionesFile.close();

            qDebug() << "✅ Relaciones actualizadas en archivo";

            // ⭐ EMITIR SEÑAL PARA ACTUALIZAR OTRAS PARTES DE LA APLICACIÓN
            emit metadatosModificados(); // Esto activará la actualización en MainWindow

        } else {
            qDebug() << "❌ Error al escribir archivo de relaciones actualizado";
        }
    }
}
void VistaDiseno::on_campoEditado(QTableWidgetItem *item) {
    if (bloqueandoEdicion) return;
    bloqueandoEdicion = true;

    if (item->column() == 1) { // Columna de nombre
        int row = item->row();
        QString nombreAnterior;

        //Obtener el nombre anterior del campo (antes del cambio)
        if (nombresAnteriores.contains(row)) {
            nombreAnterior = nombresAnteriores[row];
        }

        QString nuevoNombre = item->text();

        // Verificar si el campo anterior estaba relacionado
        if (!nombreAnterior.isEmpty() && esCampoRelacionado(nombreAnterior)) {
            // Restaurar el nombre anterior
            item->setText(nombreAnterior);
            bloqueandoEdicion = false;
            return;
        }

        // Evitar nombres vacíos
        if (nuevoNombre.trimmed().isEmpty()) {
            item->setText(nombreAnterior.isEmpty() ? "Campo" + QString::number(row + 1)
                                                   : nombreAnterior);
            bloqueandoEdicion = false;
            return;
        }

        // Detectar si realmente cambió el nombre
        if (nombreAnterior != nuevoNombre) {
            qDebug() << "📝 Nombre de campo cambió:" << nombreAnterior << "→" << nuevoNombre;

            // Actualizar el nombre anterior para la próxima vez
            nombresAnteriores[row] = nuevoNombre;

            // Guardar inmediatamente al cambiar nombre
            guardarMetadatos();
        }
    }
    bloqueandoEdicion = false;
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
        emit tablaActualizada(nombreTablaActual);


    } catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Error al guardar", e.what());
    }

    guardandoMetadatos = false;
}
void VistaDiseno::onCellDoubleClicked(int row, int column)
{
    if (column == 1) { // Columna "Nombre del Campo"
        QTableWidgetItem *item = tablaCampos->item(row, column);
        if (!item) return;

        QString nombreCampo = item->text();
        if (esCampoRelacionado(nombreCampo)) {
            QMessageBox::warning(this, "Campo relacionado",
                                 "No se puede modificar campos relacionados a otras tablas.");
        }
    }
}


