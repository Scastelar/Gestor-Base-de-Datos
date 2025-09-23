#include "reportewidget.h"
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
ReporteWidget::ReporteWidget(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Reporte de base de datos (MiniAccess)");
    setMinimumSize(600, 500);

    resumen = new QTextEdit(this);
    resumen->setReadOnly(true);

    QFont fontResumen;
    fontResumen.setPointSize(12);
    fontResumen.setBold(true);
    resumen->setFont(fontResumen);

    resumen->setStyleSheet(
        "QTextEdit { "
        "padding: 14px; "
        "background-color: #fafafa; "
        "line-height: 240%; "    // separación de renglones
        "}"
        );

    detalle = new QTableWidget(this);
    detalle->setColumnCount(4);
    detalle->setHorizontalHeaderLabels({"Tabla", "Campos", "PK", "Registros"});
    detalle->horizontalHeader()->setStretchLastSection(true);

    QFont fontDetalle;
    fontDetalle.setPointSize(11);
    detalle->setFont(fontDetalle);

    detalle->verticalHeader()->setDefaultSectionSize(30);

    btnCopiar = new QPushButton("Copiar", this);
    btnCerrar = new QPushButton("Cerrar", this);

    btnCopiar->setStyleSheet("QPushButton { font-size: 11pt; padding: 6px 14px; }");
    btnCerrar->setStyleSheet("QPushButton { font-size: 11pt; padding: 6px 14px; }");

    QHBoxLayout *botones = new QHBoxLayout();
    botones->addStretch();
    botones->addWidget(btnCopiar);
    botones->addWidget(btnCerrar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(resumen);
    mainLayout->addWidget(detalle);
    mainLayout->addLayout(botones);

    connect(btnCerrar, &QPushButton::clicked, this, &QDialog::close);
    connect(btnCopiar, &QPushButton::clicked, [=]() {
        QString texto = resumen->toPlainText();
        QApplication::clipboard()->setText(texto);
    });
}

QString ReporteWidget::generarTextoResumen(const QVector<Metadata> &metadatos) {
    int totalTablas = metadatos.size();
    int totalCampos = 0;
    int totalPK = 0;
    int totalRegistros = 0;

    for (const Metadata &m : metadatos) {
        totalCampos += m.campos.size();
        for (const Campo &c : m.campos) {
            if (c.esPK) totalPK++;
        }
        totalRegistros += m.registros.size();
    }

    // Leer relaciones desde relationships.dat (texto plano)
    int totalRelaciones = 0;
    QFile relFile(QDir::currentPath() + "/relationships.dat");
    if (relFile.exists() && relFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&relFile);
        while (!in.atEnd()) {
            QString linea = in.readLine().trimmed();
            if (!linea.isEmpty()) {
                QStringList partes = linea.split("|");
                if (partes.size() == 4) {
                    totalRelaciones++;
                }
            }
        }
        relFile.close();
    }

    QString texto;
    texto += QString("• Tablas: %1\n").arg(totalTablas);
    texto += QString("• Campos totales: %1 · Claves primarias: %2\n")
                 .arg(totalCampos).arg(totalPK);
    texto += QString("• Registros totales (todas las tablas): %1\n").arg(totalRegistros);
    texto += QString("• Relaciones: %1\n").arg(totalRelaciones);

    return texto;
}



void ReporteWidget::generarReporte(const QVector<Metadata> &metadatos) {
    resumen->setText(generarTextoResumen(metadatos));

    detalle->setRowCount(0);
    int fila = 0;
    for (const Metadata &m : metadatos) {
        detalle->insertRow(fila);

        detalle->setItem(fila, 0, new QTableWidgetItem(m.nombreTabla));
        detalle->setItem(fila, 1, new QTableWidgetItem(QString::number(m.campos.size())));

        int pkCount = 0;
        for (const Campo &c : m.campos) {
            if (c.esPK) pkCount++;
        }
        detalle->setItem(fila, 2, new QTableWidgetItem(QString::number(pkCount)));
        detalle->setItem(fila, 3, new QTableWidgetItem(QString::number(m.registros.size())));
        fila++;
    }
}
