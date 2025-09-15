#include "vistadatos.h"
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
VistaDatos::VistaDatos(QWidget *parent)
    : QWidget(parent), indiceActual(-1), ultimoID(0), filaExpandida(-1), relacionExpandida(false)
{
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(this);

    // Crear tabla para registros - SOLO 1 COLUMNA INICIAL: *
    tablaRegistros = new QTableWidget(1, 1, this); // Cambiado a 1 columna inicial

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

    // Headers simplificados - SOLO COLUMNA "*"
    QStringList headers = {"*"};
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
    connect(tablaRegistros, &QTableWidget::cellChanged, this, &VistaDatos::onCellChanged);
    connect(tablaRegistros, &QTableWidget::currentCellChanged, this, &VistaDatos::onCurrentCellChanged);
    connect(tablaRegistros, &QTableWidget::cellDoubleClicked, this, &VistaDatos::onCellDoubleClicked);

    // 🔹 Nueva señal para expandir/contraer relación
    connect(tablaRegistros, &QTableWidget::cellClicked, this, &VistaDatos::expandirContraerRelacion);

    // Botón para agregar registro
    QPushButton *btnAgregar = new QPushButton("Agregar registro");
    connect(btnAgregar, &QPushButton::clicked, this, &VistaDatos::agregarRegistro);

    layoutPrincipal->addWidget(tablaRegistros);
    layoutPrincipal->addWidget(contenedorRelacionado);
    layoutPrincipal->addWidget(btnAgregar);

    // Configurar fila inicial
    agregarRegistro();
}



// 🔹 Nuevo: Slot para expandir/contraer relación
void VistaDatos::expandirContraerRelacion(int fila, int columna)
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
void VistaDatos::cargarRelaciones(const QString &archivoRelaciones)
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
QMap<QString, QString> VistaDatos::obtenerRelacionParaCampo(const QString &nombreCampo) const
{
    for (const auto &relacion : relaciones) {
        if (relacion["campoOrigen"] == nombreCampo) {
            return relacion;
        }
    }
    return QMap<QString, QString>(); // Retorna mapa vacío si no encuentra relación
}

// 🔹 Nuevo: Método para verificar si es relación M:M (ninguna es PK)
bool VistaDatos::esRelacionMuchosAMuchos(const QMap<QString, QString> &relacion) const
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
void VistaDatos::agregarBotonRelacion(int fila, int columna)
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
void VistaDatos::onBotonRelacionClicked(int fila, int columna)
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
void VistaDatos::mostrarTablaRelacionada(const QString &tablaDestino, const QString &campoOrigen, const QString &valor)
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
void VistaDatos::onDatosRelacionadosRecibidos(const QList<QMap<QString, QVariant>> &datos)
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

void VistaDatos::mostrarSelectorFecha(int row, int col, const QString &formato)
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

QString VistaDatos::formatearFechaSegunFormato(const QDate &fecha, const QString &formato) const
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

void VistaDatos::configurarEditorFecha(QTableWidgetItem *item, const QString &formato)
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

QString VistaDatos::formatearMoneda(double valor, const QString &simbolo) const
{
    if (simbolo == "Lempira") {
        return QString("Lps %1").arg(valor, 0, 'f', 2);
    } else if (simbolo == "Dólar") {
        return QString("$%1").arg(valor, 0, 'f', 2);
    } else if (simbolo == "Euros") {
        return QString("€%1").arg(valor, 0, 'f', 2);
    } else if (simbolo == "Millares") {
        return QString("₡%1").arg(valor, 0, 'f', 2);
    }
    return QString::number(valor, 'f', 2);
}

QString VistaDatos::formatearFecha(const QDateTime &fecha, const QString &formato) const
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

void VistaDatos::configurarTablaRegistros()
{
    // Configurar solo la columna básica inicial (asterisco)
    tablaRegistros->setColumnWidth(0, 30);  // Columna del asterisco

    // Columna del asterisco (no editable)
    QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
    asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
    asteriscoItem->setTextAlignment(Qt::AlignCenter);
    tablaRegistros->setItem(0, 0, asteriscoItem);

    // NO crear columna ID aquí - se agregará dinámicamente con la metadata si es necesario

    // Marcar la fila actual con asterisco
    indiceActual = 0;
    QTableWidgetItem *currentAsterisco = tablaRegistros->item(0, 0);
    if (currentAsterisco) {
        currentAsterisco->setText("*");
        currentAsterisco->setToolTip("Fila actual");
    }
}

void VistaDatos::actualizarAsteriscoIndice(int nuevaFila, int viejaFila)
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

void VistaDatos::onCurrentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    Q_UNUSED(currentColumn);
    Q_UNUSED(previousColumn);

    actualizarAsteriscoIndice(currentRow, previousRow);
}

void VistaDatos::onCellChanged(int row, int column)
{
    if (column >= 1) { // Si cambió un campo de datos (columna 0 es *, 1+ son campos)
        QTableWidgetItem *item = tablaRegistros->item(row, column);
        if (item && item->text().isEmpty()) {
            // Para campos MONEDA, establecer "0.00" en lugar de "Valor"
            int campoIndex = column - 1; // -1 porque columna 0 es *
            if (campoIndex >= 0 && campoIndex < camposMetadata.size()) {
                const Campo &campo = camposMetadata[campoIndex];
                QString texto = item->text();
                if (campo.tipo == "MONEDA") {
                    // Eliminar símbolos y comas antes de convertir a double
                    texto.remove("Lps").remove("$").remove("€").remove("₡").remove(",");
                    double valor = texto.toDouble();
                    QString simbolo = campo.obtenerPropiedad().toString();
                    item->setText(formatearMoneda(valor, simbolo));
                }   else if (campo.propiedad == "decimal" || campo.propiedad == "doble"){
                    double valor = texto.toDouble();

                    item->setText(QString::number(valor, 'f', 2));
                    }
            } else {
                item->setText("Valor");
            }
        }
        emit registroModificado(row);
    }
}

void VistaDatos::establecerPK()
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

int VistaDatos::obtenerFilaPK() const
{
    for (int row = 0; row < tablaRegistros->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaRegistros->item(row, 0);
        if (pkItem && pkItem->toolTip().contains("Llave Primaria")) {
            return row;
        }
    }
    return -1;
}

QString VistaDatos::obtenerNombrePK() const
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

QList<QMap<QString, QVariant>> VistaDatos::obtenerRegistros(const QVector<Campo> &campos) const
{
    QList<QMap<QString, QVariant>> registros;

    // Asumir que las primeras columnas son de control
    // Columna 0: "*" (selección), Columna 1: ID
    // El resto de las columnas son para los campos definidos en 'campos'
    int offset = 1; // El offset es 1 porque el ID se maneja por separado y está en la primera columna de datos.

    // El loop principal debería iterar sobre las filas de la tabla
    for (int row = 0; row < tablaRegistros->rowCount(); ++row) {
        QMap<QString, QVariant> registro;

        // Iterar sobre los campos para obtener los valores de las columnas
        for (int col = 0; col < campos.size(); ++col) {
            const Campo &campo = campos[col];
            QTableWidgetItem *item = tablaRegistros->item(row, col + offset);

            if (item) {
                QVariant valor;
                QString texto = item->text();

                if (campo.tipo == "TEXTO") {
                    valor = texto;
                }
                else if (campo.tipo == "NUMERO") {
                    QString subTipo = campo.obtenerPropiedad().toString();
                    if (subTipo == "entero") {
                        valor = texto.toInt();
                    } else if (subTipo == "decimal" || subTipo == "doble") {
                        valor = texto.trimmed().toDouble();
                    } else if (subTipo == "byte") {
                        bool ok;
                        int byteValor = texto.toInt(&ok);
                        if (ok && byteValor >= 0 && byteValor <= 255) {
                            valor = byteValor;
                        } else {
                            valor = 0; // Valor por defecto en caso de error
                        }
                    }
                }
                else if (campo.tipo == "MONEDA") {
                    texto.remove("Lps").remove("$").remove("€").remove("₡").remove(",");
                    valor = texto.trimmed().toDouble();
                }
                else if (campo.tipo == "FECHA") {
                    // Lógica para FECHA
                    QString formato = campo.obtenerPropiedad().toString();
                    QDateTime fecha;

                    // Si el formato es 'DD/MES/YYYY'
                    if (formato == "DD/MES/YYYY") {
                        QLocale locale(QLocale::Spanish); // Usar el locale apropiado
                        fecha = locale.toDateTime(texto, "dd/MMMM/yyyy");
                    } else {
                        // Para los otros formatos, usa los formatos estándar de Qt
                        QString qtFormat;
                        if (formato == "DD-MM-YY") qtFormat = "dd-MM-yy";
                        else if (formato == "DD/MM/YY") qtFormat = "dd/MM/yy";
                        else if (formato == "YYYY-MM-DD") qtFormat = "yyyy-MM-dd";
                        fecha = QDateTime::fromString(texto, qtFormat);
                    }

                    if (fecha.isValid()) valor = fecha;
                    else valor = QDateTime::currentDateTime();
                } else {
                    valor = texto;
                }

                registro[campo.nombre] = valor;
            }
        }
        registros.append(registro);
    }
    return registros;
}

void VistaDatos::onCellDoubleClicked(int row, int column)
{
    if (column < 1) return;

    if (camposMetadata.isEmpty()) return;

    int campoIndex = column - 1;
    if (campoIndex < 0 || campoIndex >= camposMetadata.size()) return;

    const Campo &campo = camposMetadata[campoIndex];
    QTableWidgetItem *item = tablaRegistros->item(row, column);

    if (campo.tipo == "TEXTO") {
        // Obtener el límite de caracteres del campo
        int maxCaracteres = campo.obtenerPropiedad().toInt();
        if (maxCaracteres <= 0) { // Si no se ha definido, usar un valor por defecto
            maxCaracteres = 255;
        }

        QLineEdit *lineEdit = new QLineEdit(tablaRegistros);
        lineEdit->setMaxLength(maxCaracteres);
        lineEdit->setText(item->text());
        tablaRegistros->setCellWidget(row, column, lineEdit);
        lineEdit->setFocus();

        // Conectar una señal para guardar el valor cuando el editor pierde el foco
        connect(lineEdit, &QLineEdit::editingFinished, this, [this, row, column, lineEdit]() {
            QTableWidgetItem *item = tablaRegistros->item(row, column);
            if (item) {
                item->setText(lineEdit->text());
            }
            tablaRegistros->removeCellWidget(row, column); // Quitar el editor
            emit registroModificado(row);
        });
    }
    else if (campo.tipo == "FECHA") {
        QString formato = campo.obtenerPropiedad().toString();
        mostrarSelectorFecha(row, column, formato);
    }
}

void VistaDatos::cargarDesdeMetadata(const Metadata &meta)
{
    camposMetadata = meta.campos;

    // Desconectar temporalmente
    disconnect(tablaRegistros, &QTableWidget::cellChanged, this, &VistaDatos::onCellChanged);

    // Limpiar la tabla pero mantener estructura básica de *
    tablaRegistros->setRowCount(0);
    botonesRelaciones.clear();

    // Configurar número correcto de columnas: * + campos
    tablaRegistros->setColumnCount(1 + meta.campos.size()); // * + campos

    //Configurar cabeceras
    QStringList headers;
    headers << "*"; // Columna fija
    for (const Campo &c : meta.campos) {
        headers << c.nombre;
    }
    tablaRegistros->setHorizontalHeaderLabels(headers);


    // Configurar anchos de columnas
    tablaRegistros->setColumnWidth(0, 30);  // Asterisco

    for (int i = 0; i < meta.campos.size(); i++) {
        tablaRegistros->setColumnWidth(i + 1, 150); // Campos de datos
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

        // Campos de datos (columnas 1+)
        for (int i = 0; i < meta.campos.size(); i++) {
            const Campo &campo = meta.campos[i];
            QVariant valor = registro.value(campo.nombre);

            QTableWidgetItem *item = new QTableWidgetItem();
            item->setFlags(item->flags() | Qt::ItemIsEditable);

            // Esto es crucial: formatear el texto según el tipo de campo
            if (valor.isValid()) {
                if (campo.tipo == "FECHA" && valor.canConvert<QDateTime>()) {
                    // Usar la propiedad de formato del campo para formatear la fecha
                    QString formato = campo.obtenerPropiedad().toString();
                    item->setText(formatearFecha(valor.toDateTime(), formato));
                } else {
                    item->setText(valor.toString());
                }
            } else {
                item->setText("");
            }


            // Configurar según tipo de campo...
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
                // ... código para fecha
            }
            else {
                item->setText(valor.isValid() ? valor.toString() : "");
            }

            tablaRegistros->setItem(row, i + 1, item); // +1 por *
            agregarBotonRelacion(row, i + 1); // Solo agregar botón si hay relación
        }
    }

    // Si no hay registros, agregar uno vacío
    if (tablaRegistros->rowCount() == 0) {
        agregarRegistro();
    } else {
        actualizarAsteriscoIndice(0, -1);
    }

    // Reconectar
    connect(tablaRegistros, &QTableWidget::cellChanged, this, &VistaDatos::onCellChanged);
}

// Modificar agregarRegistro para agregar botones
void VistaDatos::agregarRegistro()
{
    int row = tablaRegistros->rowCount();
    tablaRegistros->insertRow(row);

    // Asterisco (columna 0)
    QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
    asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
    asteriscoItem->setTextAlignment(Qt::AlignCenter);
    tablaRegistros->setItem(row, 0, asteriscoItem);

    // NO agregar ID automáticamente - se agregará dinámicamente con la metadata si es necesario

    // Campos de datos (columnas 1+)
    for (int i = 0; i < camposMetadata.size(); i++) {
        const Campo &campo = camposMetadata[i];
        QTableWidgetItem *item = new QTableWidgetItem();
        item->setFlags(item->flags() | Qt::ItemIsEditable);

        // Valores por defecto según tipo
        if (campo.tipo == "MONEDA") {
            QString simbolo = campo.obtenerPropiedad().toString();
            item->setText(formatearMoneda(0.00, simbolo));
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

        tablaRegistros->setItem(row, i + 1, item); // +1 por *
        agregarBotonRelacion(row, i + 1); // Solo si hay relación
    }

    QTimer::singleShot(100, this, [this, row]() {
        validarRegistroCompleto(row);
    });

    emit registroAgregado(row, "");
}

// Modificar onCellChangedValidacion para actualizar botones
void VistaDatos::onCellChangedValidacion(int row, int column)
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
void VistaDatos::validarRegistroCompleto(int fila)
{
    // Implementación básica de validación
    bool esValido = esRegistroValido(fila);
    resaltarErrores(fila, !esValido);
}

bool VistaDatos::esRegistroValido(int fila)
{
    // Implementación básica - siempre válido por ahora
    return true;
}

void VistaDatos::resaltarErrores(int fila, bool tieneErrores)
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

bool VistaDatos::validarLlavePrimariaUnica(int filaActual)
{
    // Implementación básica - siempre válido por ahora
    return true;
}

bool VistaDatos::validarTipoDato(int fila, int columna, const QString &valor)
{
    // Implementación básica - siempre válido por ahora
    return true;
}

bool VistaDatos::esValorUnicoEnColumna(int columna, const QString &valor, int filaExcluir)
{
    // Implementación básica - siempre único por ahora
    return true;
}
