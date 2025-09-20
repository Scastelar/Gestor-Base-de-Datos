#include "vistaconsulta.h"
#include <QHeaderView>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QDebug>

VistaConsulta::VistaConsulta(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    tablaResultados = new QTableWidget(this);
    tablaResultados->setEditTriggers(QAbstractItemView::NoEditTriggers);
    tablaResultados->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaResultados->setAlternatingRowColors(true);
    tablaResultados->horizontalHeader()->setStretchLastSection(true);

    layout->addWidget(tablaResultados);
    setLayout(layout);
}

void VistaConsulta::mostrarConsulta(const QString &sqlLike,
                                    const QVector<Metadata> &tablas)
{
    aplicarConsulta(sqlLike, tablas);
}

void VistaConsulta::aplicarConsulta(const QString &sqlLike,
                                    const QVector<Metadata> &tablas)
{
    qDebug() << "Ejecutando consulta:" << sqlLike;

    QString sql = sqlLike.trimmed();

    // Validar SELECT
    if (!sql.startsWith("SELECT", Qt::CaseInsensitive)) {
        QMessageBox::warning(this, "Error", "Consulta inválida (falta SELECT)");
        return;
    }

    // Buscar la posición de FROM
    int idxFrom = sql.toUpper().indexOf(" FROM ");
    if (idxFrom == -1) {
        QMessageBox::warning(this, "Error", "Consulta inválida (falta FROM)");
        return;
    }

    // Partes
    QString selectPart = sql.mid(6, idxFrom - 6).trimmed(); // entre SELECT y FROM
    QString fromPart   = sql.mid(idxFrom + 6).trimmed();    // lo que sigue al FROM

    // Separar campos
    QStringList campos = selectPart.split(",", Qt::SkipEmptyParts);
    for (QString &c : campos) c = c.trimmed();

    // Nombre de tabla (solo 1 de momento)
    QString nombreTabla = fromPart.section(' ', 0, 0).trimmed();

    // Buscar metadata
    Metadata meta;
    bool encontrada = false;
    for (const Metadata &m : tablas) {
        if (m.nombreTabla.compare(nombreTabla, Qt::CaseInsensitive) == 0) {
            meta = m;
            encontrada = true;
            break;
        }
    }

    if (!encontrada) {
        QMessageBox::warning(this, "Error", "Tabla no encontrada: " + nombreTabla);
        return;
    }

    // Abrir archivo .data
    QString dataFile = QDir::currentPath() + "/tables/" + nombreTabla + ".data";
    QFile file(dataFile);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir " + dataFile);
        return;
    }

    QTextStream in(&file);

    // Limpiar tabla ANTES de configurar columnas
    tablaResultados->clearContents();
    tablaResultados->setRowCount(0);   // 🔹 eliminar todas las filas previas
    tablaResultados->setColumnCount(campos.size());
    tablaResultados->setHorizontalHeaderLabels(campos);


    int row = 0;
    for (const auto &registro : meta.registros) {
        tablaResultados->insertRow(row);

        for (int c = 0; c < campos.size(); c++) {
            QString campo = campos[c];
            QVariant valor;

            // Buscar el campo en la metadata
            for (const Campo &campoMeta : meta.campos) {
                if (campoMeta.nombre.compare(campo, Qt::CaseInsensitive) == 0) {
                    valor = registro.value(campoMeta.nombre);
                    break;
                }
            }

            tablaResultados->setItem(row, c, new QTableWidgetItem(valor.toString()));
        }
        row++;
    }
}



