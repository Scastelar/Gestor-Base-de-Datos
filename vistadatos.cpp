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
#include <QListWidget>
#include <QTimer>
#include <QFile>
#include <QTextStream>
#include <QWidgetAction>
#include <QLabel>
#include <QDoubleValidator>
#include <QDebug>

VistaDatos::VistaDatos(QWidget *parent)
    : QWidget(parent), indiceActual(-1), ultimoID(0), filaExpandida(-1), relacionExpandida(false),
    validador(nullptr), nombreTablaActual("")
 {
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(this);
    tablaRegistros = new QTableWidget(1, 1, this);
    configurarTablaRegistros();

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

    QStringList headers = {"*"};
    tablaRegistros->setHorizontalHeaderLabels(headers);
    tablaRegistros->horizontalHeader()->setStretchLastSection(true);
    tablaRegistros->verticalHeader()->setVisible(true);
    tablaRegistros->setSelectionBehavior(QAbstractItemView::SelectItems);
    tablaRegistros->setSelectionMode(QAbstractItemView::SingleSelection);
    tablaRegistros->setAlternatingRowColors(true);
    tablaRegistros->setStyleSheet(
        "QTableWidget {gridline-color: #d0d0d0;background-color: white;}"
        "QTableWidget::item:selected {background-color: #0078d4;color: white;}"
        "QHeaderView::section {background-color: #f0f0f0;padding: 4px;border: 1px solid #d0d0d0;font-weight: bold;}"
        );

    connect(tablaRegistros, &QTableWidget::cellChanged, this, &VistaDatos::onCellChanged);
    connect(tablaRegistros, &QTableWidget::currentCellChanged, this, &VistaDatos::onCurrentCellChanged);
    connect(tablaRegistros, &QTableWidget::cellDoubleClicked, this, &VistaDatos::onCellDoubleClicked);

    connect(tablaRegistros, &QTableWidget::cellClicked, this, &VistaDatos::expandirContraerRelacion);

    QPushButton *btnAgregar = new QPushButton("Agregar registro");
    connect(btnAgregar, &QPushButton::clicked, this, &VistaDatos::agregarRegistro);

    layoutPrincipal->addWidget(tablaRegistros);
    layoutPrincipal->addWidget(contenedorRelacionado);
    layoutPrincipal->addWidget(btnAgregar);
    agregarRegistro();
    configurarValidador();
}

void VistaDatos::expandirContraerRelacion(int fila, int columna)
{
    if (!relacionExpandida || fila != filaExpandida) return;
    contenedorRelacionado->hide();
    relacionExpandida = false;
    filaExpandida = -1;
}

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
            relacion["campoDestcino"] = partes[3];
            relacion["esMuchosAMuchos"] = "true";
            relaciones.append(relacion);
        }
    }
    archivo.close();
}

QMap<QString, QString> VistaDatos::obtenerRelacionParaCampo(const QString &nombreCampo) const
{
    for (const auto &relacion : relaciones) {
        if (relacion["campoOrigen"] == nombreCampo) {
            return relacion;
        }
    }
    return QMap<QString, QString>();
}

bool VistaDatos::esRelacionMuchosAMuchos(const QMap<QString, QString> &relacion) const
{
    // ⭐ NUEVA LÓGICA: Usar ValidadorRelaciones para determinar el tipo correcto
    if (!validador) {
        qDebug() << "⚠️ No hay validador para determinar tipo de relación";
        return false; // Sin validador, asumir que NO es M:M
    }

    QString tablaOrigen = relacion["tablaOrigen"];
    QString campoOrigen = relacion["campoOrigen"];
    QString tablaDestino = relacion["tablaDestino"];
    QString campoDestino = relacion.value("campoDestino", relacion.value("campoDestcino")); // Fix typo

    // Buscar la relación en el validador
    RelacionFK rel1 = validador->obtenerRelacionFK(tablaOrigen, campoOrigen);
    RelacionFK rel2 = validador->obtenerRelacionFK(tablaDestino, campoDestino);

    // Es M:M solo si el validador confirma que es "M:M"
    bool esMMRel1 = (rel1.esValida() && rel1.tipoRelacion == "M:M");
    bool esMMRel2 = (rel2.esValida() && rel2.tipoRelacion == "M:M");

    bool resultado = esMMRel1 || esMMRel2;

    qDebug() << "🔍 Verificando M:M para" << tablaOrigen << "." << campoOrigen
             << "<->" << tablaDestino << "." << campoDestino << ":" << resultado;

    return resultado;
}

void VistaDatos::agregarBotonRelacion(int fila, int columna)
{
    if (columna < 2) return;
    int campoIndex = columna - 2;
    if (campoIndex < 0 || campoIndex >= camposMetadata.size()) return;
    const Campo &campo = camposMetadata[campoIndex];
    QMap<QString, QString> relacion = obtenerRelacionParaCampo(campo.nombre);
    if (relacion.isEmpty()) {
        if (botonesRelaciones.contains(fila) && botonesRelaciones[fila].contains(columna)) {
            QPushButton* botonExistente = botonesRelaciones[fila][columna];
            tablaRegistros->removeCellWidget(fila, columna);
            delete botonExistente;
            botonesRelaciones[fila].remove(columna);
        }
        return;
    }

    if (esRelacionMuchosAMuchos(relacion)) {
        if (botonesRelaciones.contains(fila) && botonesRelaciones[fila].contains(columna)) {
            QPushButton* botonExistente = botonesRelaciones[fila][columna];
            tablaRegistros->removeCellWidget(fila, columna);
            delete botonExistente;
            botonesRelaciones[fila].remove(columna);
        }

        QPushButton *botonRelacion = new QPushButton("🔗");
        botonRelacion->setFixedSize(20, 20);
        botonRelacion->setStyleSheet(
            "QPushButton {border: 1px solid #ccc;border-radius: 3px;background-color: #e0e0e0;}"
            "QPushButton:hover {background-color: #d0d0d0;}"
            "QPushButton:pressed {background-color: #c0c0c0;}"
            );
        botonRelacion->setToolTip("Ver registros relacionados en " + relacion["tablaDestino"]);

        connect(botonRelacion, &QPushButton::clicked, this, [this, fila, columna]() {
            onBotonRelacionClicked(fila, columna);
        });

        QWidget *widgetContenedor = new QWidget();
        QHBoxLayout *layout = new QHBoxLayout(widgetContenedor);
        layout->setContentsMargins(2, 2, 2, 2);
        layout->setSpacing(2);
        layout->addWidget(botonRelacion);
        layout->addStretch();
        tablaRegistros->setCellWidget(fila, columna, widgetContenedor);
        botonesRelaciones[fila][columna] = botonRelacion;
    }
}

void VistaDatos::onBotonRelacionClicked(int fila, int columna)
{
    if (columna < 1 || columna - 1 >= camposMetadata.size()) return;

    QString nombreCampo = camposMetadata[columna - 1].nombre;
    QMap<QString, QString> relacion = obtenerRelacionParaCampo(nombreCampo);

    if (relacion.isEmpty()) return;
    QTableWidgetItem *item = tablaRegistros->item(fila, columna);
    if (item && !item->text().isEmpty()) {
        QString valor = item->text();
        if (relacionExpandida && fila == filaExpandida) {
            contenedorRelacionado->hide();
            relacionExpandida = false;
            filaExpandida = -1;
        } else {
            filaExpandida = fila;
            relacionExpandida = true;
            mostrarTablaRelacionada(relacion["tablaDestino"], relacion["campoOrigen"], valor);
        }
    }
}

void VistaDatos::mostrarTablaRelacionada(const QString &tablaDestino, const QString &campoOrigen, const QString &valor)
{
    tablaRelacionada->setRowCount(0);
    QStringList headers;
    headers << "ID" << "Nombre" << "Valor";
    tablaRelacionada->setColumnCount(headers.size());
    tablaRelacionada->setHorizontalHeaderLabels(headers);

    int row = tablaRelacionada->rowCount();
    tablaRelacionada->insertRow(row);
    QTableWidgetItem *idItem = new QTableWidgetItem("");
    tablaRelacionada->setItem(row, 0, idItem);
    QTableWidgetItem *nombreItem = new QTableWidgetItem(campoOrigen);
    nombreItem->setFlags(nombreItem->flags() & ~Qt::ItemIsEditable);
    tablaRelacionada->setItem(row, 1, nombreItem);
    QTableWidgetItem *valorItem = new QTableWidgetItem(valor);
    tablaRelacionada->setItem(row, 2, valorItem);

    tablaRelacionada->insertRow(row + 1);
    QTableWidgetItem *botonItem = new QTableWidgetItem("+ Agregar otra relación");
    botonItem->setFlags(botonItem->flags() & ~Qt::ItemIsEditable);
    botonItem->setBackground(QBrush(QColor(200, 230, 200)));
    tablaRelacionada->setItem(row + 1, 0, botonItem);
    tablaRelacionada->setSpan(row + 1, 0, 1, 3);

    for (int i = 0; i < tablaRelacionada->columnCount(); ++i) {
        tablaRelacionada->horizontalHeader()->setSectionResizeMode(i, QHeaderView::ResizeToContents);
    }

    QLabel *titulo = qobject_cast<QLabel*>(contenedorRelacionado->layout()->itemAt(0)->widget());
    if (titulo) titulo->setText("Relaciones M:M - " + tablaDestino + " (Edición)");
    contenedorRelacionado->show();
    relacionExpandida = true;
}

void VistaDatos::onDatosRelacionadosRecibidos(const QList<QMap<QString, QVariant>> &datos)
{
    if (!relacionExpandida) return;
    tablaRelacionada->setRowCount(0);
    if (datos.isEmpty()) {
        tablaRelacionada->setRowCount(1);
        tablaRelacionada->setItem(0, 0, new QTableWidgetItem("No hay registros relacionados"));
        return;
    }

    if (!datos.isEmpty()) {
        QStringList headers = datos.first().keys();
        tablaRelacionada->setColumnCount(headers.size());
        tablaRelacionada->setHorizontalHeaderLabels(headers);
    }

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
    QCalendarWidget *calendar = new QCalendarWidget(&dialog);
    calendar->setGridVisible(true);
    calendar->setVerticalHeaderFormat(QCalendarWidget::NoVerticalHeader);

    QTableWidgetItem *item = tablaRegistros->item(row, col);
    QDate fechaActual = QDate::currentDate();
    if (item && !item->text().isEmpty()) {
        QString textoFecha = item->text();
        QDate fechaParseada = QDate::fromString(textoFecha,
                                                formato == "DD-MM-YY" ? "dd-MM-yy" :
                                                    formato == "DD/MM/YY" ? "dd/MM/yy" :
                                                    formato == "YYYY-MM-DD" ? "yyyy-MM-dd" : "dd-MM-yy");
        if (fechaParseada.isValid()) fechaActual = fechaParseada;
    }
    calendar->setSelectedDate(fechaActual);

    QHBoxLayout *quickButtonsLayout = new QHBoxLayout();
    QPushButton *btnHoy = new QPushButton("Hoy", &dialog);
    QPushButton *btnManana = new QPushButton("Mañana", &dialog);
    QPushButton *btnAyer = new QPushButton("Ayer", &dialog);

    connect(btnHoy, &QPushButton::clicked, [calendar]() { calendar->setSelectedDate(QDate::currentDate()); });
    connect(btnManana, &QPushButton::clicked, [calendar]() { calendar->setSelectedDate(QDate::currentDate().addDays(1)); });
    connect(btnAyer, &QPushButton::clicked, [calendar]() { calendar->setSelectedDate(QDate::currentDate().addDays(-1)); });

    quickButtonsLayout->addWidget(btnHoy);
    quickButtonsLayout->addWidget(btnManana);
    quickButtonsLayout->addWidget(btnAyer);
    quickButtonsLayout->addStretch();

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    layout->addLayout(quickButtonsLayout);
    layout->addWidget(calendar);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        QDate fechaSeleccionada = calendar->selectedDate();
        QString fechaFormateada = formatearFecha(fechaSeleccionada, formato);
        if (!item) {
            item = new QTableWidgetItem();
            tablaRegistros->setItem(row, col, item);
        }
        item->setText(fechaFormateada);
        emit registroModificado(row);
    }
}

QString VistaDatos::formatearFecha(const QVariant &fechaInput, const QString &formato) const
{
    QDate fecha;
    if (fechaInput.typeId() == QMetaType::QDate) {
        fecha = fechaInput.toDate();
    } else if (fechaInput.typeId() == QMetaType::QDateTime) {
        fecha = fechaInput.toDateTime().date();
    } else {
        return "";
    }

    if (formato == "DD-MM-YY") return fecha.toString("dd-MM-yy");
    if (formato == "DD/MM/YY") return fecha.toString("dd/MM/yy");
    if (formato == "YYYY-MM-DD") return fecha.toString("yyyy-MM-dd");
    if (formato == "DD/MES/YYYY") {
        static const QString meses[] = {"Enero", "Febrero", "Marzo", "Abril", "Mayo", "Junio", "Julio", "Agosto", "Septiembre", "Octubre", "Noviembre", "Diciembre"};
        return QString("%1/%2/%3").arg(fecha.day(), 2, 10, QLatin1Char('0')).arg(meses[fecha.month() - 1]).arg(fecha.year());
    }
    return fecha.toString("dd-MM-yyyy");
}

QString VistaDatos::formatearMoneda(double valor, const QString &simbolo) const
{
    static QMap<QString, QString> formatos = {
        {"Lempira", "Lps %1"},
        {"Dólar", "$%1"},
        {"Euros", "€%1"},
        {"Millares", "₡%1"}
    };
    return formatos.value(simbolo, "%1").arg(valor, 0, 'f', 2);
}

void VistaDatos::configurarTablaRegistros()
{
    tablaRegistros->setColumnWidth(0, 30);
    QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
    asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
    asteriscoItem->setTextAlignment(Qt::AlignCenter);
    tablaRegistros->setItem(0, 0, asteriscoItem);
    indiceActual = 0;
    QTableWidgetItem *currentAsterisco = tablaRegistros->item(0, 0);
    if (currentAsterisco) {
        currentAsterisco->setText("*");
        currentAsterisco->setToolTip("Fila actual");
    }
}

void VistaDatos::actualizarAsteriscoIndice(int nuevaFila, int viejaFila)
{
    if (viejaFila >= 0 && viejaFila < tablaRegistros->rowCount()) {
        QTableWidgetItem *oldAsterisco = tablaRegistros->item(viejaFila, 0);
        if (oldAsterisco) oldAsterisco->setText("");
    }
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
    if (column >= 1) {
        QTableWidgetItem *item = tablaRegistros->item(row, column);
        if (item) {
            QString valor = item->text().trimmed();

            qDebug() << "📝 Celda cambiada: fila" << row << "columna" << column
                     << "valor:" << valor << "tabla:" << nombreTablaActual;

            // 🔹 NUEVA VALIDACIÓN DE FK
            if (!validarCampoFK(row, column, valor)) {
                // Resaltar celda con error
                item->setBackground(QBrush(QColor(255, 200, 200)));
                item->setToolTip("❌ Clave foránea inválida");
                qDebug() << "❌ Validación FK falló, celda resaltada en rojo";
                return; // No procesar más si hay error FK
            } else {
                // Limpiar resaltado si el valor es válido
                item->setBackground(QBrush(Qt::white));
                item->setToolTip("");
            }

            // Tu código existente de formateo...
            int campoIndex = column - 1;
            if (campoIndex >= 0 && campoIndex < camposMetadata.size()) {
                const Campo &campo = camposMetadata[campoIndex];
                QString texto = item->text();
                if (campo.tipo == "NUMERO" && !valor.isEmpty()) {
                    bool ok;
                    valor.toInt(&ok);
                    if (!ok) {
                        item->setBackground(QBrush(QColor(255, 200, 200)));
                        item->setToolTip("Valor numérico inválido");
                        return;
                    }
                }
                else if (campo.tipo == "MONEDA") {
                    texto.remove("Lps").remove("$").remove("€").remove("₡").remove(",");
                    double valor = texto.toDouble();
                    QString simbolo = campo.obtenerPropiedad().toString();
                    item->setText(formatearMoneda(valor, simbolo));
                } else if (campo.propiedad == "decimal" || campo.propiedad == "doble") {
                    double valor = texto.toDouble();
                    item->setText(QString::number(valor, 'f', 2));
                }
            }

            agregarBotonRelacion(row, column);
            QTimer::singleShot(100, this, [this, row]() { validarRegistroCompleto(row); });
            emit registroModificado(row);
        }
    }
}

void VistaDatos::onCellDoubleClicked(int row, int column)
{
    if (column < 1) return;

    if (camposMetadata.isEmpty()) return;

    int campoIndex = column - 1;
    if (campoIndex < 0 || campoIndex >= camposMetadata.size()) return;

    const Campo &campo = camposMetadata[campoIndex];
    QTableWidgetItem *item = tablaRegistros->item(row, column);

    // 🔹 NUEVO: Verificar si es una clave foránea PRIMERO
    if (validador && validador->esCampoClaveForanea(nombreTablaActual, campo.nombre)) {
        QStringList valoresValidos = validador->obtenerValoresValidos(nombreTablaActual, campo.nombre);
        if (!valoresValidos.isEmpty()) {
            mostrarSelectorFK(row, column, valoresValidos);
            return; // Salir temprano, no mostrar otros editores
        }
    }

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

void VistaDatos::actualizarValidadorRelaciones()
{
    if (validador) {
        validador->cargarRelaciones();
        qDebug() << "🔄 ValidadorRelaciones actualizado para tabla:" << nombreTablaActual;
    } else {
        qDebug() << "⚠️ No se puede actualizar validador: no está inicializado";
    }
}

void VistaDatos::establecerNombreTabla(const QString &nombre)
{
    if (nombre.isEmpty()) {
        qDebug() << "⚠️ ADVERTENCIA: Se está estableciendo nombre de tabla vacío";
        return;
    }

    QString nombreAnterior = nombreTablaActual;
    nombreTablaActual = nombre;

    qDebug() << "📋 Nombre de tabla establecido:" << nombreAnterior << "→" << nombreTablaActual;

    // Configurar o reconfigurar validador
    if (!validador) {
        configurarValidador();
    } else {
        // Recargar relaciones con el nuevo nombre
        validador->cargarRelaciones();
        qDebug() << "🔄 Validador reconfigurado para:" << nombreTablaActual;
    }
}

void VistaDatos::establecerPK()
{
    int currentRow = indiceActual;
    if (currentRow == -1) {
        QMessageBox::information(this, "Selección requerida", "Por favor, seleccione una fila primero.");
        return;
    }

    for (int row = 0; row < tablaRegistros->rowCount(); ++row) {
        QTableWidgetItem *pkItem = tablaRegistros->item(row, 0);
        if (pkItem && pkItem->toolTip().contains("Llave Primaria")) {
            if (row == indiceActual) pkItem->setText("*");
            else pkItem->setText("");
            pkItem->setToolTip("");
        }
    }

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
        if (pkItem && pkItem->toolTip().contains("Llave Primaria")) return row;
    }
    return -1;
}

QString VistaDatos::obtenerNombrePK() const
{
    int pkRow = obtenerFilaPK();
    if (pkRow != -1) {
        QTableWidgetItem *idItem = tablaRegistros->item(pkRow, 1);
        if (idItem) return idItem->text();
    }
    return "";
}

QList<QMap<QString, QVariant>> VistaDatos::obtenerRegistros(const QVector<Campo> &campos) const
{
    QList<QMap<QString, QVariant>> registros;
    int offset = 1;

    for (int row = 0; row < tablaRegistros->rowCount(); ++row) {
        QMap<QString, QVariant> registro;
        for (int col = 0; col < campos.size(); ++col) {
            const Campo &campo = campos[col];
            QTableWidgetItem *item = tablaRegistros->item(row, col + offset);
            if (item) {
                QVariant valor;
                QString texto = item->text();
                if (campo.tipo == "TEXTO") valor = texto;
                else if (campo.tipo == "NUMERO") {
                    QString subTipo = campo.obtenerPropiedad().toString();
                    if (subTipo == "entero") valor = texto.toInt();
                    else if (subTipo == "decimal" || subTipo == "doble") valor = texto.trimmed().toDouble();
                    else if (subTipo == "byte") {
                        bool ok;
                        int byteValor = texto.toInt(&ok);
                        valor = (ok && byteValor >= 0 && byteValor <= 255) ? byteValor : 0;
                    }
                } else if (campo.tipo == "MONEDA") {
                    texto.remove("Lps").remove("$").remove("€").remove("₡").remove(",");
                    valor = texto.trimmed().toDouble();
                } else if (campo.tipo == "FECHA") {
                    QString formato = campo.obtenerPropiedad().toString();
                    QDateTime fecha;
                    if (formato == "DD/MES/YYYY") {
                        QLocale locale(QLocale::Spanish);
                        fecha = locale.toDateTime(texto, "dd/MMMM/yyyy");
                    } else {
                        QString qtFormat = formato == "DD-MM-YY" ? "dd-MM-yy" :
                                               formato == "DD/MM/YY" ? "dd/MM/yy" :
                                               formato == "YYYY-MM-DD" ? "yyyy-MM-dd" : "dd-MM-yy";
                        fecha = QDateTime::fromString(texto, qtFormat);
                    }
                    valor = fecha.isValid() ? fecha : QDateTime::currentDateTime();
                } else valor = texto;
                registro[campo.nombre] = valor;
            }
        }
        registros.append(registro);
    }
    return registros;
}

void VistaDatos::configurarCelda(int fila, int columna, const QVariant &valor, const Campo &campo)
{
    QTableWidgetItem *item = new QTableWidgetItem();
    item->setFlags(item->flags() | Qt::ItemIsEditable);

    if (campo.tipo == "TEXTO") configurarCeldaTexto(item, valor, campo);
    else if (campo.tipo == "MONEDA") configurarCeldaMoneda(item, valor, campo);
    else if (campo.tipo == "FECHA") configurarCeldaFecha(item, valor, campo);
    else if (campo.tipo == "NUMERO") configurarCeldaNumero(item, valor, campo);
    else item->setText(valor.isValid() ? valor.toString() : "");

    tablaRegistros->setItem(fila, columna, item);
}

void VistaDatos::configurarCeldaTexto(QTableWidgetItem *item, const QVariant &valor, const Campo &campo)
{
    item->setText(valor.isValid() ? valor.toString() : "");
}

void VistaDatos::configurarCeldaMoneda(QTableWidgetItem *item, const QVariant &valor, const Campo &campo)
{
    if (valor.isValid()) {
        double valorNum = valor.toDouble();
        QString simbolo = campo.obtenerPropiedad().toString();
        item->setText(formatearMoneda(valorNum, simbolo));
    } else {
        item->setText(formatearMoneda(0.00, campo.obtenerPropiedad().toString()));
    }
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
}

void VistaDatos::configurarCeldaFecha(QTableWidgetItem *item, const QVariant &valor, const Campo &campo)
{
    if (valor.isValid()) {
        QString formato = campo.obtenerPropiedad().toString();
        item->setText(formatearFecha(valor, formato));
    } else {
        QString formato = campo.obtenerPropiedad().toString();
        item->setText(formatearFecha(QDateTime::currentDateTime(), formato));
    }
}

void VistaDatos::configurarCeldaNumero(QTableWidgetItem *item, const QVariant &valor, const Campo &campo)
{
    QString subTipo = campo.obtenerPropiedad().toString();
    QVariant valorValido = valor.isValid() ? valor : 0;

    if (subTipo == "entero") {
        int v = valorValido.toInt();
        item->setText(QString::number(v));
    }
    else if (subTipo == "decimal") {
        double v = valorValido.toDouble();
        item->setText(QString::number(v, 'f', 2));
    }
    else if (subTipo == "doble") {
        double v = valorValido.toDouble();
        item->setText(QString::number(v, 'g', 15));
    }
    else if (subTipo == "byte") {
        int v = valorValido.toInt();
        if (v < 0 || v > 255) v = 0;
        item->setText(QString::number(v));
    }
    else {
        item->setText(valorValido.toString());
    }

    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
}


void VistaDatos::cargarDesdeMetadata(const Metadata &meta)
{
    camposMetadata = meta.campos;
    disconnect(tablaRegistros, &QTableWidget::cellChanged, this, &VistaDatos::onCellChanged);

    tablaRegistros->setRowCount(0);
    botonesRelaciones.clear();
    tablaRegistros->setColumnCount(1 + meta.campos.size());

    QStringList headers;
    headers << "*";
    for (const Campo &c : meta.campos) headers << c.nombre;
    tablaRegistros->setHorizontalHeaderLabels(headers);

    tablaRegistros->setColumnWidth(0, 30);
    for (int i = 0; i < meta.campos.size(); i++) tablaRegistros->setColumnWidth(i + 1, 150);

    indiceActual = -1;
    ultimoID = 0;

    for (const auto &registro : meta.registros) {
        int row = tablaRegistros->rowCount();
        tablaRegistros->insertRow(row);

        QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
        asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
        asteriscoItem->setTextAlignment(Qt::AlignCenter);
        tablaRegistros->setItem(row, 0, asteriscoItem);

        for (int i = 0; i < meta.campos.size(); i++) {
            const Campo &campo = meta.campos[i];
            QVariant valor = registro.value(campo.nombre);
            configurarCelda(row, i + 1, valor, campo);
        }
    }

    if (tablaRegistros->rowCount() == 0){
        agregarRegistro();
    }
    else {
        actualizarAsteriscoIndice(0, -1);
    }

     // ⭐ DEBUG: Mostrar estado final
    if (validador) {
        validador->debugRelaciones(nombreTablaActual);
    }

    connect(tablaRegistros, &QTableWidget::cellChanged, this, &VistaDatos::onCellChanged);
}

void VistaDatos::eliminarRegistro() {

    int currentRow = tablaRegistros->currentRow();
    if (currentRow == -1) {
        QMessageBox::information(this, "Selección requerida", "Por favor, seleccione un campo para eliminar.");
        return;
    }

    tablaRegistros->removeRow(currentRow);
}

void VistaDatos::agregarRegistro()
{
    int row = tablaRegistros->rowCount();
    tablaRegistros->insertRow(row);

    QTableWidgetItem *asteriscoItem = new QTableWidgetItem();
    asteriscoItem->setFlags(asteriscoItem->flags() & ~Qt::ItemIsEditable);
    asteriscoItem->setTextAlignment(Qt::AlignCenter);
    tablaRegistros->setItem(row, 0, asteriscoItem);

    for (int i = 0; i < camposMetadata.size(); i++) {
        const Campo &campo = camposMetadata[i];
        QVariant valor;
        if (campo.tipo == "MONEDA") valor = 0.00;
        else if (campo.tipo == "FECHA") valor = QDateTime::currentDateTime();
        else if (campo.tipo == "NUMERO") valor = 0;
        configurarCelda(row, i + 1, valor, campo);
    }

    QTimer::singleShot(100, this, [this, row]() { validarRegistroCompleto(row); });
    emit registroAgregado(row, "");
}

void VistaDatos::validarRegistroCompleto(int fila)
{
    bool esValido = esRegistroValido(fila);
    resaltarErrores(fila, !esValido);
}

bool VistaDatos::esRegistroValido(int fila) { return true; }

void VistaDatos::resaltarErrores(int fila, bool tieneErrores)
{
    for (int col = 0; col < tablaRegistros->columnCount(); ++col) {
        QTableWidgetItem *item = tablaRegistros->item(fila, col);
        if (item) item->setBackground(QBrush(tieneErrores ? QColor(255, 200, 200) : Qt::white));
    }
}

bool VistaDatos::validarLlavePrimariaUnica(int filaActual) { return true; }
bool VistaDatos::validarTipoDato(int fila, int columna, const QString &valor) { return true; }
bool VistaDatos::esValorUnicoEnColumna(int columna, const QString &valor, int filaExcluir) { return true; }

// En vistadatos.cpp
void VistaDatos::ordenar(Qt::SortOrder order)
{
    int columnaActual = tablaRegistros->currentColumn();

    // La columna 0 es el asterisco/llave, la 1 es el ID si existe
    if (columnaActual <= 0) {
        return;
    }

    int campoIndex = columnaActual - 1;
    if (campoIndex < 0 || campoIndex >= camposMetadata.size()) {
        return;
    }

    const Campo &campo = camposMetadata[campoIndex];
    QString tipoCampo = campo.tipo.toUpper();

    Qt::SortOrder ordenFinal = order;

    // Invertir el orden para FECHA si la dirección es descendente
    if (tipoCampo == "FECHA" && order == Qt::DescendingOrder) {
        ordenFinal = Qt::DescendingOrder; // Mantener descendente para 'reciente a viejo'
    } else if (tipoCampo == "FECHA" && order == Qt::AscendingOrder) {
        ordenFinal = Qt::AscendingOrder; // Mantener ascendente para 'viejo a reciente'
    } else {
        // Para TEXTO, NUMERO, MONEDA y otros tipos, el comportamiento es el mismo
        ordenFinal = order;
    }

    // Desconectar temporalmente la señal para evitar que el ordenamiento dispare onCellChanged
    disconnect(tablaRegistros, &QTableWidget::cellChanged, this, &VistaDatos::onCellChanged);

    // Realizar el ordenamiento de la tabla
    tablaRegistros->sortItems(columnaActual, ordenFinal);

    // Reconectar la señal
    connect(tablaRegistros, &QTableWidget::cellChanged, this, &VistaDatos::onCellChanged);
}

void VistaDatos::configurarValidador()
{
    if (nombreTablaActual.isEmpty()) {
        qDebug() << "❌ ERROR: No se puede configurar validador sin nombre de tabla";
        return;
    }

    if (!validador) {
        validador = new ValidadorRelaciones(this);
        qDebug() << "✅ ValidadorRelaciones creado para tabla:" << nombreTablaActual;
    } else {
        qDebug() << "🔄 ValidadorRelaciones ya existía, recargando relaciones...";
    }

    // Forzar recarga de relaciones
    validador->cargarRelaciones();
}

// NUEVO método para validar FK
bool VistaDatos::validarCampoFK(int fila, int columna, const QString &valor)
{
    if (!validador) {
        qDebug() << "⚠️ validarCampoFK: Validador no inicializado";
        return true; // No validar si no hay validador
    }

    if (nombreTablaActual.isEmpty()) {
        qDebug() << "⚠️ validarCampoFK: Nombre de tabla vacío";
        return true;
    }

    if (columna < 1 || columna - 1 >= camposMetadata.size()) {
        return true;
    }

    int campoIndex = columna - 1;
    const Campo &campo = camposMetadata[campoIndex];

    // Debug detallado
    qDebug() << "🔍 Validando FK en tabla:" << nombreTablaActual
             << "campo:" << campo.nombre
             << "valor:" << valor;

    // Verificar si es una clave foránea usando el validador
    bool esCampoFK = validador->esCampoClaveForanea(nombreTablaActual, campo.nombre);
    if (!esCampoFK) {
        qDebug() << "ℹ️ Campo" << campo.nombre << "no es FK, validación pasada";
        return true; // No es FK, permitir cualquier valor
    }

    qDebug() << "🔗 Campo" << campo.nombre << "ES una FK, validando valor...";

    bool esValido = validador->validarClaveForanea(nombreTablaActual, campo.nombre, valor);

    if (!esValido && !valor.isEmpty()) {
        QStringList valoresValidos = validador->obtenerValoresValidos(nombreTablaActual, campo.nombre);
        RelacionFK relacion = validador->obtenerRelacionFK(nombreTablaActual, campo.nombre);

        QString mensaje = QString("❌ CLAVE FORÁNEA INVÁLIDA\n\n"
                                  "Tabla: %1\nCampo: %2\nValor: '%3'\n\n"
                                  "El valor no existe en la tabla '%4' campo '%5'")
                              .arg(nombreTablaActual)
                              .arg(campo.nombre)
                              .arg(valor)
                              .arg(relacion.tablaPrincipal)
                              .arg(relacion.campoPrincipal);

        qDebug() << "❌ FK inválida:" << mensaje;
        mostrarErrorValidacionFK(mensaje, valoresValidos);
    } else if (esValido) {
        qDebug() << "✅ FK válida:" << valor;
    }

    return esValido;
}

void VistaDatos::mostrarErrorValidacionFK(const QString &mensaje, const QStringList &valoresValidos)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle("Error de Integridad Referencial");
    msgBox.setText(mensaje);

    if (!valoresValidos.isEmpty() && valoresValidos.size() <= 10) {
        QString detalle = "Valores válidos disponibles:\n• " + valoresValidos.join("\n• ");
        msgBox.setDetailedText(detalle);
    } else if (valoresValidos.size() > 10) {
        QString detalle = QString("Hay %1 valores válidos disponibles.\n"
                                  "Primeros 10: %2...")
                              .arg(valoresValidos.size())
                              .arg(valoresValidos.mid(0, 10).join(", "));
        msgBox.setDetailedText(detalle);
    }

    msgBox.setStandardButtons(QMessageBox::Ok);
    msgBox.exec();
}


void VistaDatos::mostrarSelectorFK(int fila, int columna, const QStringList &valoresValidos)
{
    QDialog dialog(this);
    dialog.setWindowTitle("Seleccionar Valor de Clave Foránea");
    dialog.resize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *lblInfo = new QLabel("Seleccione un valor válido:");
    layout->addWidget(lblInfo);

    QListWidget *lista = new QListWidget(&dialog);
    lista->addItems(valoresValidos);
    lista->setSortingEnabled(true);
    layout->addWidget(lista);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    // Doble clic para seleccionar
    connect(lista, &QListWidget::itemDoubleClicked, [&dialog]() {
        dialog.accept();
    });

    if (dialog.exec() == QDialog::Accepted) {
        QListWidgetItem *itemSeleccionado = lista->currentItem();
        if (itemSeleccionado) {
            QTableWidgetItem *celda = tablaRegistros->item(fila, columna);
            if (!celda) {
                celda = new QTableWidgetItem();
                tablaRegistros->setItem(fila, columna, celda);
            }
            celda->setText(itemSeleccionado->text());
            emit registroModificado(fila);
        }
    }
}

