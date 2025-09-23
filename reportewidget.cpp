#include "reportewidget.h"
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
ReporteWidget::ReporteWidget(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Reporte de base de datos (MiniAccess)");
    setMinimumSize(500, 400);

    resumen = new QTextEdit(this);
    resumen->setReadOnly(true);

    detalle = new QTableWidget(this);
    detalle->setColumnCount(4);
    detalle->setHorizontalHeaderLabels({"Tabla", "Campos", "PK", "Registros"});
    detalle->horizontalHeader()->setStretchLastSection(true);

    btnCopiar = new QPushButton("Copiar", this);
    btnCerrar = new QPushButton("Cerrar", this);

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

    QString texto;
    texto += QString("• Tablas: %1\n").arg(totalTablas);
    texto += QString("• Campos totales: %1 · Claves primarias: %2 · Índices sin duplicados: 0\n")
                 .arg(totalCampos).arg(totalPK);
    texto += QString("• Registros totales (todas las tablas): %1\n").arg(totalRegistros);
    texto += QString("• Relaciones: (por ahora 0)\n"); //  Aquí puedes integrar con tu relacioneswidget
    texto += QString("• Consultas guardadas: 0\n");
    texto += QString("• Formularios guardados: 0\n");
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
