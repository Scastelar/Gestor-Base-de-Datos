#include "datasheetwidget.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QDateTimeEdit>
#include <QCalendarWidget>
#include <QToolButton>
#include <QHBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>

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
    // Conectar señal de doble clic
    connect(tablaRegistros, &QTableWidget::cellDoubleClicked, this, &DataSheetWidget::onCellDoubleClicked);


    // Botón para agregar registro
    QPushButton *btnAgregar = new QPushButton("Agregar registro");
    connect(btnAgregar, &QPushButton::clicked, this, &DataSheetWidget::agregarRegistro);

    layoutPrincipal->addWidget(tablaRegistros);
    layoutPrincipal->addWidget(btnAgregar);

    // Configurar fila inicial
    agregarRegistro();
}

void DataSheetWidget::mostrarSelectorFecha(int row, int col, const QString &formato)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Seleccionar Fecha");
    dialog.setModal(true);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    // Calendario
    QCalendarWidget *calendar = new QCalendarWidget(&dialog);
    calendar->setGridVisible(true);
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    // Intentar obtener la fecha actual del campo
    QTableWidgetItem *item = tablaRegistros->item(row, col);
    QDate fechaActual = QDate::currentDate();

    if (item && !item->text().isEmpty()) {
        // Intentar parsear la fecha existente según el formato
        QString textoFecha = item->text();
        QDate fechaParseada;

        if (formato == "DD-MM-YY") {
            fechaParseada = QDate::fromString(textoFecha, "dd-MM-yy");
        } else if (formato == "DD/MM/YY") {
            fechaParseada = QDate::fromString(textoFecha, "dd/MM/yy");
        } else if (formato == "YYYY-MM-DD") {
            fechaParseada = QDate::fromString(textoFecha, "yyyy-MM-dd");
        }

        if (fechaParseada.isValid()) {
            fechaActual = fechaParseada;
        }
    }

    calendar->setSelectedDate(fechaActual);

    // Botones de acción rápida
    QHBoxLayout *quickButtonsLayout = new QHBoxLayout();

    QPushButton *btnHoy = new QPushButton("Hoy", &dialog);
    QPushButton *btnManana = new QPushButton("Mañana", &dialog);
    QPushButton *btnAyer = new QPushButton("Ayer", &dialog);

    connect(btnHoy, &QPushButton::clicked, [calendar]() {
        calendar->setSelectedDate(QDate::currentDate());
    });

    connect(btnManana, &QPushButton::clicked, [calendar]() {
        calendar->setSelectedDate(QDate::currentDate().addDays(1));
    });

    connect(btnAyer, &QPushButton::clicked, [calendar]() {
        calendar->setSelectedDate(QDate::currentDate().addDays(-1));
    });

    quickButtonsLayout->addWidget(btnHoy);
    quickButtonsLayout->addWidget(btnManana);
    quickButtonsLayout->addWidget(btnAyer);
    quickButtonsLayout->addStretch();

    // Botones de diálogo
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);

    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    // Ensamblar layout
    layout->addLayout(quickButtonsLayout);
    layout->addWidget(calendar);
    layout->addWidget(buttonBox);

    // Mostrar diálogo
    if (dialog.exec() == QDialog::Accepted) {
        QDate fechaSeleccionada = calendar->selectedDate();
        QString fechaFormateada = formatearFechaSegunFormato(fechaSeleccionada, formato);

        if (!item) {
            item = new QTableWidgetItem();
            tablaRegistros->setItem(row, col, item);
        }

        item->setText(fechaFormateada);
        emit registroModificado(row);
    }
}

QString DataSheetWidget::formatearFechaSegunFormato(const QDate &fecha, const QString &formato) const
{
    if (formato == "DD-MM-YY") {
        return fecha.toString("dd-MM-yy");
    } else if (formato == "DD/MM/YY") {
        return fecha.toString("dd/MM/yy");
    } else if (formato == "DD/MES/YYYY") {
        static const QString meses[] = {
            "Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio",
            "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"
        };

        return QString("%1/%2/%3")
            .arg(fecha.day(), 2, 10, QLatin1Char('0'))
            .arg(meses[fecha.month() - 1])
            .arg(fecha.year());
    } else if (formato == "YYYY-MM-DD") {
        return fecha.toString("yyyy-MM-dd");
    }

    // Formato por defecto
    return fecha.toString("dd-MM-yyyy");
}

void DataSheetWidget::configurarEditorFecha(QTableWidgetItem *item, const QString &formato)
{
    int row = item->row();
    int col = item->column();

    // Crear editor de fecha
    QDateTimeEdit *dateEditor = new QDateTimeEdit();
    dateEditor->setCalendarPopup(true);
    dateEditor->setDisplayFormat("yyyy-MM-dd"); // Formato por defecto para edición

    // Establecer fecha actual si el campo está vacío
    if (item->text().isEmpty()) {
        dateEditor->setDateTime(QDateTime::currentDateTime());
    } else {
        // Intentar parsear la fecha existente
        QDateTime fecha = QDateTime::fromString(item->text(), Qt::ISODate);
        if (!fecha.isValid()) {
            fecha = QDateTime::currentDateTime();
        }
        dateEditor->setDateTime(fecha);
    }

    tablaRegistros->setCellWidget(row, col, dateEditor);

    // Conectar la señal de cambio de fecha
    connect(dateEditor, &QDateTimeEdit::dateTimeChanged, this, [this, row, col, formato](const QDateTime &dateTime) {
        // Cuando cambia la fecha, actualizar el ítem con el formato correcto
        QTableWidgetItem *item = tablaRegistros->item(row, col);
        if (item) {
            item->setText(formatearFecha(dateTime, formato));
        }
    });
}

QString DataSheetWidget::formatearFecha(const QDateTime &fecha, const QString &formato) const
{
    if (formato == "DD-MM-YY") {
        return fecha.toString("dd-MM-yy");
    } else if (formato == "DD/MM/YY") {
        return fecha.toString("dd/MM/yy");
    } else if (formato == "DD/MES/YYYY") {
        QString mes;
        switch (fecha.date().month()) {
        case 1: mes = "Enero"; break;
        case 2: mes = "Febrero"; break;
        case 3: mes = "Marzo"; break;
        case 4: mes = "Abril"; break;
        case 5: mes = "Mayo"; break;
        case 6: mes = "Junio"; break;
        case 7: mes = "Julio"; break;
        case 8: mes = "Agosto"; break;
        case 9: mes = "Septiembre"; break;
        case 10: mes = "Octubre"; break;
        case 11: mes = "Noviembre"; break;
        case 12: mes = "Diciembre"; break;
        default: mes = "Enero";
        }
        return QString("%1/%2/%3").arg(fecha.date().day(), 2, 10, QLatin1Char('0'))
            .arg(mes)
            .arg(fecha.date().year());
    } else if (formato == "YYYY-MM-DD") {
        return fecha.toString("yyyy-MM-dd");
    }

    // Formato por defecto
    return fecha.toString("dd-MM-yyyy");
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
    if (column >= 2) { // Si cambió un campo de datos
        QTableWidgetItem *item = tablaRegistros->item(row, column);
        if (item && item->text().isEmpty()) {
            // Para campos MONEDA, establecer "0.00" en lugar de "Valor"
            int campoIndex = column - 2;
            if (campoIndex >= 0 && campoIndex < camposMetadata.size()) {
                const Campo &campo = camposMetadata[campoIndex];
                if (campo.tipo == "MONEDA") {
                    item->setText("0.00");
                } else {
                    item->setText("Valor");
                }
            } else {
                item->setText("Valor");
            }
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

        // Obtener el ID
        QTableWidgetItem *idItem = tablaRegistros->item(row, 1);
        if (idItem) {
            registro["ID"] = idItem->text().toInt();
        }

        for (int col = 0; col < campos.size(); ++col) {
            const Campo &campo = campos[col];

            // Saltar el campo ID ya que lo manejamos arriba
            if (campo.nombre == "ID") continue;

            QTableWidgetItem *item = tablaRegistros->item(row, col + 2); // +2 por *, ID

            if (item) {
                QVariant valor;

                if (campo.tipo == "TEXTO") {
                    valor = item->text();
                }
                else if (campo.tipo == "NUMERO") {
                    valor = item->text().toDouble();
                }
                else if (campo.tipo == "MONEDA") {
                    // Para moneda, guardar solo el valor numérico (sin símbolo)
                    QString texto = item->text();

                    // Remover símbolos de moneda comunes para obtener el valor numérico
                    texto.remove("Lps");
                    texto.remove("$");
                    texto.remove("€");
                    texto.remove("₡");
                    texto.remove(","); // Remover separadores de miles
                    texto = texto.trimmed();

                    bool ok;
                    double valorNum = texto.toDouble(&ok);
                    valor = ok ? valorNum : 0.0;
                }
                else if (campo.tipo == "FECHA") {
                    // ... (código existente para FECHA)
                    QString formato = campo.obtenerPropiedad().toString();
                    QString textoFecha = item->text();
                    QDateTime fecha;

                    if (formato == "DD-MM-YY") {
                        fecha = QDateTime::fromString(textoFecha, "dd-MM-yy");
                    } else if (formato == "DD/MM/YY") {
                        fecha = QDateTime::fromString(textoFecha, "dd/MM/yy");
                    } else if (formato == "DD/MES/YYYY") {
                        // Manejar formato con nombre de mes
                        QStringList partes = textoFecha.split('/');
                        if (partes.size() == 3) {
                            int dia = partes[0].toInt();
                            QString mesStr = partes[1];
                            int año = partes[2].toInt();

                            int mes = 1;
                            if (mesStr == "Enero") mes = 1;
                            else if (mesStr == "Febrero") mes = 2;
                            else if (mesStr == "Marzo") mes = 3;
                            else if (mesStr == "Abril") mes = 4;
                            else if (mesStr == "Mayo") mes = 5;
                            else if (mesStr == "Junio") mes = 6;
                            else if (mesStr == "Julio") mes = 7;
                            else if (mesStr == "Agosto") mes = 8;
                            else if (mesStr == "Septiembre") mes = 9;
                            else if (mesStr == "Octubre") mes = 10;
                            else if (mesStr == "Noviembre") mes = 11;
                            else if (mesStr == "Diciembre") mes = 12;

                            fecha = QDateTime(QDate(año, mes, dia), QTime(0, 0));
                        }
                    } else if (formato == "YYYY-MM-DD") {
                        fecha = QDateTime::fromString(textoFecha, "yyyy-MM-dd");
                    }

                    if (!fecha.isValid()) {
                        fecha = QDateTime::currentDateTime();
                    }

                    valor = fecha;
                }
                else {
                    valor = item->text();
                }

                registro[campo.nombre] = valor;
            }
        }

        registros.append(registro);
    }

    return registros;
}


void DataSheetWidget::cargarDesdeMetadata(const Metadata &meta)
{
    // Almacenar los campos metadata
    camposMetadata = meta.campos; // Agrega esta línea

    // Desconectar temporalmente para evitar señales durante la carga
    disconnect(tablaRegistros, &QTableWidget::cellChanged, this, &DataSheetWidget::onCellChanged);
    // Limpiar la tabla pero mantener la estructura básica
    tablaRegistros->setRowCount(0);

    // Configurar columnas según los campos de metadata
    tablaRegistros->setColumnCount(meta.campos.size() + 2); // +2 para * e ID

    QStringList headers;
    headers << "*" << "ID";
    for (const Campo &c : meta.campos) {
        headers << c.nombre;
    }
    tablaRegistros->setHorizontalHeaderLabels(headers);

    // Configurar anchos de columnas
    tablaRegistros->setColumnWidth(0, 30);  // Columna del asterisco
    tablaRegistros->setColumnWidth(1, 80);  // Columna del ID

    for (int i = 0; i < meta.campos.size(); i++) {
        tablaRegistros->setColumnWidth(i + 2, 150); // Ancho para campos de datos
    }

    ultimoID = 0;
    indiceActual = -1;

    // 🔹 Cargar registros desde Metadata
    for (const auto &registro : meta.registros) {
        int row = tablaRegistros->rowCount();
        tablaRegistros->insertRow(row);

        // Asterisco
        QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
        asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
        asteriscoItem->setTextAlignment(Qt::AlignCenter);
        tablaRegistros->setItem(row, 0, asteriscoItem);

        // ID - usar el ID real del registro si existe, sino autoincremental
        QVariant idValor = registro.value("ID");
        int id = idValor.isValid() ? idValor.toInt() : ++ultimoID;

        QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(id));
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        idItem->setTextAlignment(Qt::AlignCenter);
        tablaRegistros->setItem(row, 1, idItem);

        if (id > ultimoID) {
            ultimoID = id; // Mantener el último ID actualizado
        }

        // Campos de datos
        for (int i = 0; i < meta.campos.size(); i++) {
            const Campo &campo = meta.campos[i];
            QVariant valor = registro.value(campo.nombre);

            // Si el campo es ID, ya lo manejamos arriba, saltar
            if (campo.nombre == "ID") continue;

            QTableWidgetItem *item = new QTableWidgetItem();

            if (campo.tipo == "TEXTO") {
                item->setText(valor.isValid() ? valor.toString() : "");
            }
            else if (campo.tipo == "NUMERO") {
                item->setText(valor.isValid() ? QString::number(valor.toDouble()) : "0");
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            else if (campo.tipo == "MONEDA") {
                if (valor.isValid()) {
                    double valorNum = valor.toDouble();
                    QString simbolo = campo.obtenerPropiedad().toString();
                    QString textoMoneda;

                    if (simbolo == "Lempira") {
                        textoMoneda = QString("Lps %1").arg(valorNum, 0, 'f', 2);
                    }
                    else if (simbolo == "Dólar") {
                        textoMoneda = QString("$%1").arg(valorNum, 0, 'f', 2);
                    }
                    else if (simbolo == "Euros") {
                        textoMoneda = QString("€%1").arg(valorNum, 0, 'f', 2);
                    }
                    else if (simbolo == "Millares") {
                        textoMoneda = QString("₡%1").arg(valorNum, 0, 'f', 2);
                    }
                    else {
                        textoMoneda = QString::number(valorNum, 'f', 2);
                    }

                    item->setText(textoMoneda);
                } else {
                    item->setText("0.00");
                }
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            else if (campo.tipo == "FECHA") {
                QString formatoFecha = campo.obtenerPropiedad().toString();
                if (valor.isValid()) {
                    QDateTime fecha = valor.toDateTime();
                    item->setText(formatearFecha(fecha, formatoFecha));
                } else {
                    item->setText("");
                }
            }
            else {
                item->setText(valor.isValid() ? valor.toString() : "");
            }

            tablaRegistros->setItem(row, i + 2, item);
        }
    }

    // Si no hay registros, agregar uno vacío
    if (tablaRegistros->rowCount() == 0) {
        agregarRegistro();
    } else {
        // Marcar la primera fila como actual
        actualizarAsteriscoIndice(0, -1);
    }

    // Reconectar la señal
    connect(tablaRegistros, &QTableWidget::cellChanged, this, &DataSheetWidget::onCellChanged);
}

// Modificar el método agregarRegistro para inicializar campos FECHA
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

    // Campos de datos - valores por defecto
    for (int col = 2; col < tablaRegistros->columnCount(); col++) {
        int campoIndex = col - 2;
        QTableWidgetItem *item = new QTableWidgetItem("");

        // Inicializar campos MONEDA con "0.00"
        if (campoIndex >= 0 && campoIndex < camposMetadata.size()) {
            const Campo &campo = camposMetadata[campoIndex];
            if (campo.tipo == "MONEDA") {
                item->setText("0.00");
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
        }

        tablaRegistros->setItem(row, col, item);
    }

    emit registroAgregado(ultimoID, "");
}

void DataSheetWidget::onCellDoubleClicked(int row, int column)
{
    if (column < 2) return; // No editar columnas * e ID

    // Verificar si tenemos metadata de campos
    if (camposMetadata.isEmpty()) {
        return;
    }

    int campoIndex = column - 2;
    if (campoIndex < 0 || campoIndex >= camposMetadata.size()) {
        return;
    }

    const Campo &campo = camposMetadata[campoIndex];
    QTableWidgetItem *item = tablaRegistros->item(row, column);

    if (campo.tipo == "FECHA") {
        QString formato = campo.obtenerPropiedad().toString();
        mostrarSelectorFecha(row, column, formato);
    }
    else if (campo.tipo == "MONEDA") {
        // Para moneda, editar solo el valor numérico (sin símbolo)
        if (item) {
            QString texto = item->text();

            // Remover símbolos de moneda para edición
            texto.remove("Lps");
            texto.remove("$");
            texto.remove("€");
            texto.remove("₡");
            texto.remove(",");
            texto = texto.trimmed();

            // Crear editor para el valor numérico
            QLineEdit *editor = new QLineEdit(texto);
            editor->setValidator(new QDoubleValidator(0, 999999999, 2, editor));
            editor->selectAll();

            connect(editor, &QLineEdit::editingFinished, this, [this, row, column, &campo, editor]() {
                QString nuevoTexto = editor->text();
                bool ok;
                double valor = nuevoTexto.toDouble(&ok);

                if (ok) {
                    QString simbolo = campo.obtenerPropiedad().toString();
                    QString textoMoneda;

                    if (simbolo == "Lempira") {
                        textoMoneda = QString("Lps %1").arg(valor, 0, 'f', 2);
                    }
                    else if (simbolo == "Dólar") {
                        textoMoneda = QString("$%1").arg(valor, 0, 'f', 2);
                    }
                    else if (simbolo == "Euros") {
                        textoMoneda = QString("€%1").arg(valor, 0, 'f', 2);
                    }
                    else if (simbolo == "Millares") {
                        textoMoneda = QString("₡%1").arg(valor, 0, 'f', 2);
                    }
                    else {
                        textoMoneda = QString::number(valor, 'f', 2);
                    }

                    QTableWidgetItem *item = tablaRegistros->item(row, column);
                    if (item) {
                        item->setText(textoMoneda);
                        emit registroModificado(row);
                    }
                }

                tablaRegistros->setCellWidget(row, column, nullptr);
                editor->deleteLater();
            });

            tablaRegistros->setCellWidget(row, column, editor);
            editor->setFocus();
        }
    }
}




