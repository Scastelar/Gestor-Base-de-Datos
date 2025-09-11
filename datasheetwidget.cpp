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
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QWidgetAction>
#include <QLabel>
#include <QDoubleValidator>
#include <QDebug>


// Constructor modificado
DataSheetWidget::DataSheetWidget(QWidget *parent)
    : QWidget(parent), indiceActual(-1), ultimoID(0), filaExpandida(-1), relacionExpandida(false)
{
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(this);

    // Crear tabla para registros - SOLO 2 COLUMNAS INICIALES: * e ID
    tablaRegistros = new QTableWidget(1, 2, this); // Cambiado a 2 columnas iniciales

    // Headers iniciales simplificados - SOLO * e ID


    // Resto del constructor permanece igual...
    configurarTablaRegistros();

    // Crear contenedor para tabla relacionada (inicialmente oculto)
    contenedorRelacionado = new QWidget(this);
    QVBoxLayout *layoutRelacionado = new QVBoxLayout(contenedorRelacionado);

    tablaRelacionada = new QTableWidget(0, 3, contenedorRelacionado);
    tablaRelacionada->setHorizontalHeaderLabels({"ID", "Campo", "Valor"});
    tablaRelacionada->horizontalHeader()->setStretchLastSection(true);
    tablaRelacionada->setAlternatingRowColors(true);

    QPushButton *btnCerrarRelacion = new QPushButton("Cerrar", contenedorRelacionado);
    connect(btnCerrarRelacion, &QPushButton::clicked, this, [this]() {
        contenedorRelacionado->hide();
        relacionExpandida = false;
        filaExpandida = -1;
    });

    layoutRelacionado->addWidget(new QLabel("Registros Relacionados:"));
    layoutRelacionado->addWidget(tablaRelacionada);
    layoutRelacionado->addWidget(btnCerrarRelacion);
    contenedorRelacionado->hide();

    // Headers simplificados
    QStringList headers = {"*", "ID"}; // Solo estas dos columnas iniciales
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
    connect(tablaRegistros, &QTableWidget::cellDoubleClicked, this, &DataSheetWidget::onCellDoubleClicked);

    // 🔹 Nueva señal para expandir/contraer relación
    connect(tablaRegistros, &QTableWidget::cellClicked, this, &DataSheetWidget::expandirContraerRelacion);

    // Botón para agregar registro
    QPushButton *btnAgregar = new QPushButton("Agregar registro");
    connect(btnAgregar, &QPushButton::clicked, this, &DataSheetWidget::agregarRegistro);

    layoutPrincipal->addWidget(tablaRegistros);
    layoutPrincipal->addWidget(contenedorRelacionado);
    layoutPrincipal->addWidget(btnAgregar);

    // Configurar fila inicial
    agregarRegistro();

}



// 🔹 Nuevo: Slot para expandir/contraer relación
void DataSheetWidget::expandirContraerRelacion(int fila, int columna)
{
    if (!relacionExpandida || fila != filaExpandida) {
        return;
    }

    // Si se hace clic en la misma fila que tiene la relación expandida, contraer
    contenedorRelacionado->hide();
    relacionExpandida = false;
    filaExpandida = -1;
}

// 🔹 Modificado: Método para cargar relaciones
void DataSheetWidget::cargarRelaciones(const QString &archivoRelaciones)
{
    relaciones.clear();
    QFile archivo(archivoRelaciones);

    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "No se pudo abrir el archivo de relaciones:" << archivoRelaciones;
        return;
    }

    QTextStream in(&archivo);
    while (!in.atEnd()) {
        QString linea = in.readLine().trimmed();
        if (linea.isEmpty() || linea.startsWith("#")) continue;

        QStringList partes = linea.split("|");
        if (partes.size() == 4) {
            QMap<QString, QString> relacion;
            relacion["tablaOrigen"] = partes[0];
            relacion["campoOrigen"] = partes[1];
            relacion["tablaDestino"] = partes[2];
            relacion["campoDestino"] = partes[3];
            relacion["esMuchosAMuchos"] = "true"; // Por ahora asumimos que todas son M:M

            relaciones.append(relacion);
            qDebug() << "Relación cargada:" << relacion["tablaOrigen"] << "->" << relacion["tablaDestino"];
        }
    }
    archivo.close();
}

// 🔹 Nuevo método: Obtener relación para un campo específico
QMap<QString, QString> DataSheetWidget::obtenerRelacionParaCampo(const QString &nombreCampo) const
{
    for (const auto &relacion : relaciones) {
        if (relacion["campoOrigen"] == nombreCampo) {
            return relacion;
        }
    }
    return QMap<QString, QString>(); // Retorna mapa vacío si no encuentra relación
}

// 🔹 Nuevo: Método para verificar si es relación M:M (ninguna es PK)
bool DataSheetWidget::esRelacionMuchosAMuchos(const QMap<QString, QString> &relacion) const
{
    // En una relación M:M, generalmente:
    // 1. Ninguno de los campos es PK en su tabla original
    // 2. O se usa una tabla intermedia (pero para simplificar, verificamos que no sean PKs)

    // Por ahora, asumimos que si el campo termina con "_id" o "ID" no es PK
    // Esto es una simplificación - en una implementación real deberías verificar la metadata
    QString campoOrigen = relacion["campoOrigen"];
    QString campoDestino = relacion["campoDestino"];

    // Si alguno de los campos parece ser una PK, no es M:M
    if (campoOrigen.compare("id", Qt::CaseInsensitive) == 0 ||
        campoOrigen.endsWith("_id", Qt::CaseInsensitive) ||
        campoDestino.compare("id", Qt::CaseInsensitive) == 0 ||
        campoDestino.endsWith("_id", Qt::CaseInsensitive)) {
        return false;
    }

    return true;
}

// 🔹 Modificado: Método para agregar botones de relación solo para M:M
void DataSheetWidget::agregarBotonRelacion(int fila, int columna)
{
    // Verificar que sea una columna de datos (no *, no ID)
    if (columna < 2) { // Columnas 0 (*) y 1 (ID) no deben tener botones
        return;
    }

    int campoIndex = columna - 2; // -2 porque columnas 0 y 1 son fijas
    if (campoIndex < 0 || campoIndex >= camposMetadata.size()) {
        return;
    }

    const Campo &campo = camposMetadata[campoIndex];

    // Buscar relaciones para este campo
    QMap<QString, QString> relacion = obtenerRelacionParaCampo(campo.nombre);

    if (relacion.isEmpty()) {
        // Limpiar botón existente si ya no hay relación
        if (botonesRelaciones.contains(fila) && botonesRelaciones[fila].contains(columna)) {
            QPushButton* botonExistente = botonesRelaciones[fila][columna];
            tablaRegistros->removeCellWidget(fila, columna);
            delete botonExistente;
            botonesRelaciones[fila].remove(columna);
        }
        return;
    }

    // 🔹 CAMBIO IMPORTANTE: Solo agregar botón para relaciones M:M
    if (esRelacionMuchosAMuchos(relacion)) {
        qDebug() << "Encontrada relación M:M para campo:" << campo.nombre
                 << "->" << relacion["tablaDestino"];

        // Limpiar botón existente si hay uno
        if (botonesRelaciones.contains(fila) && botonesRelaciones[fila].contains(columna)) {
            QPushButton* botonExistente = botonesRelaciones[fila][columna];
            tablaRegistros->removeCellWidget(fila, columna);
            delete botonExistente;
            botonesRelaciones[fila].remove(columna);
        }

        // Crear botón de relación
        QPushButton *botonRelacion = new QPushButton("🔗");
        botonRelacion->setFixedSize(20, 20);
        botonRelacion->setStyleSheet(
            "QPushButton {"
            "   border: 1px solid #ccc;"
            "   border-radius: 3px;"
            "   background-color: #e0e0e0;"
            "}"
            "QPushButton:hover {"
            "   background-color: #d0d0d0;"
            "}"
            "QPushButton:pressed {"
            "   background-color: #c0c0c0;"
            "}"
            );
        botonRelacion->setToolTip("Ver registros relacionados en " + relacion["tablaDestino"]);

        // Conectar señal
        connect(botonRelacion, &QPushButton::clicked, this,
                [this, fila, columna]() {
                    onBotonRelacionClicked(fila, columna);
                });

        // Usar QWidget para embeber el botón en la celda
        QWidget *widgetContenedor = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widgetContenedor);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(2);
        layout->addWidget(botonRelacion);
        layout->addStretch();

        tablaRegistros->setCellWidget(fila, columna, widgetContenedor);

        // Guardar referencia al botón
        botonesRelaciones[fila][columna] = botonRelacion;
    }
}

// 🔹 Modificado: Slot para manejar clic en botón de relación
void DataSheetWidget::onBotonRelacionClicked(int fila, int columna)
{
    // Obtener el nombre del campo desde la metadata
    if (columna < 1 || columna - 1 >= camposMetadata.size()) return;

    QString nombreCampo = camposMetadata[columna - 1].nombre;

    // Obtener la relación para este campo
    QMap<QString, QString> relacion = obtenerRelacionParaCampo(nombreCampo);
    if (relacion.isEmpty()) return;

    // Obtener el valor de la celda
    QTableWidgetItem *item = tablaRegistros->item(fila, columna);
    if (item && !item->text().isEmpty()) {
        QString valor = item->text();

        // Expandir/contraer
        if (relacionExpandida && fila == filaExpandida) {
            contenedorRelacionado->hide();
            relacionExpandida = false;
            filaExpandida = -1;
        } else {
            filaExpandida = fila;
            relacionExpandida = true;

            // 🔹 NUEVO: Mostrar título y campos para edición
            mostrarTablaRelacionada(relacion["tablaDestino"], relacion["campoOrigen"], valor);
        }
    }
}

// 🔹 Modificado: Método para mostrar tabla relacionada con campos editables
void DataSheetWidget::mostrarTablaRelacionada(const QString &tablaDestino, const QString &campoOrigen, const QString &valor)
{
    // Limpiar tabla existente
    tablaRelacionada->setRowCount(0);

    // 🔹 NUEVO: Configurar tabla con campos editables
    // (En una implementación real, esto debería venir de la metadata de la tabla destino)
    QStringList headers;
    headers << "ID" << "Nombre" << "Valor";
    tablaRelacionada->setColumnCount(headers.size());
    tablaRelacionada->setHorizontalHeaderLabels(headers);

    // 🔹 NUEVO: Agregar fila vacía para edición
    int row = tablaRelacionada->rowCount();
    tablaRelacionada->insertRow(row);

    // Celda de ID (vacía para nuevo registro)
    QTableWidgetItem *idItem = new QTableWidgetItem("");
    tablaRelacionada->setItem(row, 0, idItem);

    // Celda de Nombre (no editable, muestra el campo de relación)
    QTableWidgetItem *nombreItem = new QTableWidgetItem(campoOrigen);
    nombreItem->setFlags(nombreItem->flags() & ~Qt::ItemIsEditable);
    tablaRelacionada->setItem(row, 1, nombreItem);

    // Celda de Valor (editable, muestra el valor actual)
    QTableWidgetItem *valorItem = new QTableWidgetItem(valor);
    tablaRelacionada->setItem(row, 2, valorItem);

    // 🔹 NUEVO: Agregar botón para agregar más relaciones
    tablaRelacionada->insertRow(row + 1);
    QTableWidgetItem *botonItem = new QTableWidgetItem("+ Agregar otra relación");
    botonItem->setFlags(botonItem->flags() & ~Qt::ItemIsEditable);
    botonItem->setBackground(QBrush(QColor(200, 230, 200))); // Verde claro
    tablaRelacionada->setItem(row + 1, 0, botonItem);
    tablaRelacionada->setSpan(row + 1, 0, 1, 3); // Unir celdas

    // Ajustar columnas
    for (int i = 0; i < tablaRelacionada->columnCount(); ++i) {
        tablaRelacionada->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    // Mostrar contenedor con título
    QLabel *titulo = qobject_cast<QLabel*>(contenedorRelacionado->layout()->itemAt(0)->widget());
    if (titulo) {
        titulo->setText("Relaciones M:M - " + tablaDestino + " (Edición)");
    }

    contenedorRelacionado->show();
    relacionExpandida = true;
}


// El resto del código permanece igual...

// 🔹 Nuevo: Slot para recibir datos relacionados (conectar en el padre)
void DataSheetWidget::onDatosRelacionadosRecibidos(const QList<QMap<QString, QVariant>> &datos)
{
    if (!relacionExpandida) return;

    tablaRelacionada->setRowCount(0);

    if (datos.isEmpty()) {
        tablaRelacionada->setRowCount(1);
        tablaRelacionada->setItem(0, 0, new QTableWidgetItem("No hay registros relacionados"));
        return;
    }

    // Configurar columnas basadas en las keys del primer registro
    if (!datos.isEmpty()) {
        QStringList headers = datos.first().keys();
        tablaRelacionada->setColumnCount(headers.size());
        tablaRelacionada->setHorizontalHeaderLabels(headers);
    }

    // Llenar tabla con datos
    for (int i = 0; i < datos.size(); ++i) {
        const QMap<QString, QVariant> &registro = datos[i];
        int row = tablaRelacionada->rowCount();
        tablaRelacionada->insertRow(row);

        int col = 0;
        for (const QString &key : registro.keys()) {
            QTableWidgetItem *item = new QTableWidgetItem(registro[key].toString());
            tablaRelacionada->setItem(row, col, item);
            col++;
        }
    }

    // Ajustar columnas
    for (int i = 0; i < tablaRelacionada->columnCount(); ++i) {
        tablaRelacionada->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    contenedorRelacionado->show();
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
    // Configurar solo las columnas básicas iniciales
    tablaRegistros->setColumnWidth(0, 30);  // Columna del asterisco
    tablaRegistros->setColumnWidth(1, 80);  // Columna del ID

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

    // NO agregar "Campo1" aquí - se agregará dinámicamente con la metadata

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

        // Obtener el ID (columna 1)
        QTableWidgetItem *idItem = tablaRegistros->item(row, 1);
        if (idItem) {
            registro["ID"] = idItem->text().toInt();
        }

        // Obtener los campos de datos (columnas 2 en adelante)
        for (int col = 0; col < campos.size(); ++col) {
            const Campo &campo = campos[col];
            if (campo.nombre == "ID") continue; // Ya manejamos el ID

            QTableWidgetItem *item = tablaRegistros->item(row, col + 2); // +2 por *, ID

            if (item) {
                QVariant valor;
                QString texto = item->text();

                if (campo.tipo == "TEXTO") {
                    valor = texto;
                }
                else if (campo.tipo == "NUMERO") {
                    valor = texto.toDouble();
                }
                else if (campo.tipo == "MONEDA") {
                    texto.remove("Lps").remove("$").remove("€").remove("₡").remove(",");
                    valor = texto.trimmed().toDouble();
                }
                else if (campo.tipo == "FECHA") {
                    QString formato = campo.obtenerPropiedad().toString();
                    QDateTime fecha;

                    if (formato == "DD-MM-YY") fecha = QDateTime::fromString(texto, "dd-MM-yy");
                    else if (formato == "DD/MM/YY") fecha = QDateTime::fromString(texto, "dd/MM/yy");
                    else if (formato == "YYYY-MM-DD") fecha = QDateTime::fromString(texto, "yyyy-MM-dd");
                    else if (formato == "DD/MES/YYYY") {
                        // Parsear formato con nombre de mes
                        QStringList partes = texto.split('/');
                        if (partes.size() == 3) {
                            // ... lógica de parsing existente
                        }
                    }

                    if (fecha.isValid()) valor = fecha;
                    else valor = QDateTime::currentDateTime();
                }
                else {
                    valor = texto;
                }

                registro[campo.nombre] = valor;
            }
        }

        registros.append(registro);
    }

    return registros;
}

void DataSheetWidget::onCellDoubleClicked(int row, int column)
{
    if (column < 2) return; // No editar columnas * e ID

    if (camposMetadata.isEmpty()) return;

    int campoIndex = column - 2; // -2 por * e ID
    if (campoIndex < 0 || campoIndex >= camposMetadata.size()) return;

    const Campo &campo = camposMetadata[campoIndex];
    QTableWidgetItem *item = tablaRegistros->item(row, column);

    if (campo.tipo == "FECHA") {
        QString formato = campo.obtenerPropiedad().toString();
        mostrarSelectorFecha(row, column, formato);
    }
    else if (campo.tipo == "MONEDA") {
        // Código existente para moneda...
    }
}

void DataSheetWidget::cargarDesdeMetadata(const Metadata &meta)
{
    camposMetadata = meta.campos;

    // Desconectar temporalmente
    disconnect(tablaRegistros, &QTableWidget::cellChanged, this, &DataSheetWidget::onCellChanged);

    // Limpiar la tabla pero mantener estructura básica de * e ID
    tablaRegistros->setRowCount(0);
    botonesRelaciones.clear();

    // Configurar número correcto de columnas: * + ID + campos
    tablaRegistros->setColumnCount(2 + meta.campos.size()); // * + ID + campos

    QStringList headers;
    headers << "*" << "ID"; // Columnas fijas
    for (const Campo &c : meta.campos) {
        headers << c.nombre;
    }
    tablaRegistros->setHorizontalHeaderLabels(headers);

    // Configurar anchos de columnas
    tablaRegistros->setColumnWidth(0, 30);  // Asterisco
    tablaRegistros->setColumnWidth(1, 80);  // ID

    for (int i = 0; i < meta.campos.size(); i++) {
        tablaRegistros->setColumnWidth(i + 2, 150); // Campos de datos
    }

    indiceActual = -1;
    ultimoID = 0;

    // Cargar registros
    for (const auto &registro : meta.registros) {
        int row = tablaRegistros->rowCount();
        tablaRegistros->insertRow(row);

        // Asterisco (columna 0)
        QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
        asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
        asteriscoItem->setTextAlignment(Qt::AlignCenter);
        tablaRegistros->setItem(row, 0, asteriscoItem);

        // ID (columna 1)
        QTableWidgetItem *idItem = new QTableWidgetItem();
        idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
        idItem->setTextAlignment(Qt::AlignCenter);

        QVariant idValor = registro.value("ID");
        if (idValor.isValid()) {
            int id = idValor.toInt();
            idItem->setText(QString::number(id));
            if (id > ultimoID) ultimoID = id;
        } else {
            idItem->setText(QString::number(++ultimoID));
        }
        tablaRegistros->setItem(row, 1, idItem);

        // Campos de datos (columnas 2+)
        for (int i = 0; i < meta.campos.size(); i++) {
            const Campo &campo = meta.campos[i];
            QVariant valor = registro.value(campo.nombre);

            QTableWidgetItem *item = new QTableWidgetItem();
            item->setFlags(item->flags() | Qt::ItemIsEditable);

            // Configurar según tipo de campo...
            if (campo.tipo == "TEXTO") {
                item->setText(valor.isValid() ? valor.toString() : "");
            }
            else if (campo.tipo == "NUMERO") {
                item->setText(valor.isValid() ? QString::number(valor.toDouble()) : "0");
                item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            }
            else if (campo.tipo == "MONEDA") {
                // ... código para moneda
            }
            else if (campo.tipo == "FECHA") {
                // ... código para fecha
            }
            else {
                item->setText(valor.isValid() ? valor.toString() : "");
            }

            tablaRegistros->setItem(row, i + 2, item); // +2 por * e ID
            agregarBotonRelacion(row, i + 2); // Solo agregar botón si hay relación
        }
    }

    // Si no hay registros, agregar uno vacío
    if (tablaRegistros->rowCount() == 0) {
        agregarRegistro();
    } else {
        actualizarAsteriscoIndice(0, -1);
    }

    // Reconectar
    connect(tablaRegistros, &QTableWidget::cellChanged, this, &DataSheetWidget::onCellChanged);
}

// Modificar agregarRegistro para agregar botones
void DataSheetWidget::agregarRegistro()
{
    int row = tablaRegistros->rowCount();
    tablaRegistros->insertRow(row);

    // Asterisco (columna 0)
    QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
    asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
    asteriscoItem->setTextAlignment(Qt::AlignCenter);
    tablaRegistros->setItem(row, 0, asteriscoItem);

    // ID (columna 1)
    QTableWidgetItem *idItem = new QTableWidgetItem(QString::number(++ultimoID));
    idItem->setFlags(idItem->flags() & ~Qt::ItemIsEditable);
    idItem->setTextAlignment(Qt::AlignCenter);
    tablaRegistros->setItem(row, 1, idItem);

    // Campos de datos (columnas 2+)
    for (int i = 0; i < camposMetadata.size(); i++) {
        const Campo &campo = camposMetadata[i];
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        // Valores por defecto según tipo
        if (campo.tipo == "MONEDA") {
            item->setText("0.00");
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
        else if (campo.tipo == "FECHA") {
            QString formato = campo.obtenerPropiedad().toString();
            item->setText(formatearFecha(QDateTime::currentDateTime(), formato));
        }
        else if (campo.tipo == "NUMERO") {
            item->setText("0");
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        }
        else {
            item->setText("");
        }

        tablaRegistros->setItem(row, i + 2, item); // +2 por * e ID
        agregarBotonRelacion(row, i + 2); // Solo si hay relación
    }

    QTimer::singleShot(100, this, [this, row]() {
        validarRegistroCompleto(row);
    });

    emit registroAgregado(row, "");
}

// Modificar onCellChangedValidacion para actualizar botones
void DataSheetWidget::onCellChangedValidacion(int row, int column)
{
    // Validar solo si es una columna de datos (no el asterisco)
    if (column >= 1) {
        // 🔹 Actualizar botón de relación si es necesario
        agregarBotonRelacion(row, column);

        // Validar el registro completo después de un breve delay
        QTimer::singleShot(100, this, [this, row]() {
            validarRegistroCompleto(row);
        });
    }
}

// Métodos de validación (implementaciones básicas)
void DataSheetWidget::validarRegistroCompleto(int fila)
{
    // Implementación básica de validación
    bool esValido = esRegistroValido(fila);
    resaltarErrores(fila, !esValido);
}

bool DataSheetWidget::esRegistroValido(int fila)
{
    // Implementación básica - siempre válido por ahora
    return true;
}

void DataSheetWidget::resaltarErrores(int fila, bool tieneErrores)
{
    // Implementación básica - resaltar fila si tiene errores
    for (int col = 0; col < tablaRegistros->columnCount(); ++col) {
        QTableWidgetItem *item = tablaRegistros->item(fila, col);
        if (item) {
            if (tieneErrores) {
                item->setBackground(QBrush(QColor(255, 200, 200))); // Rojo claro
            } else {
                item->setBackground(QBrush(Qt::white)); // Blanco
            }
        }
    }
}

bool DataSheetWidget::validarLlavePrimariaUnica(int filaActual)
{
    // Implementación básica - siempre válido por ahora
    return true;
}

bool DataSheetWidget::validarTipoDato(int fila, int columna, const QString &valor)
{
    // Implementación básica - siempre válido por ahora
    return true;
}

bool DataSheetWidget::esValorUnicoEnColumna(int columna, const QString &valor, int filaExcluir)
{
    // Implementación básica - siempre único por ahora
    return true;
}
