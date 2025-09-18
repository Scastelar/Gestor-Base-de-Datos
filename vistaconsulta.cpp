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

    // Configurar columnas
    tablaResultados->clear();
    tablaResultados->setColumnCount(campos.size());
    tablaResultados->setHorizontalHeaderLabels(campos);

    int row = 0;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty()) continue;

        QStringList values = line.split("|"); // ⚠️ cambia si usas otro separador
        tablaResultados->insertRow(row);

        for (int c = 0; c < campos.size(); c++) {
            QString campo = campos[c];
            int idxCampo = -1;

            for (int i = 0; i < meta.campos.size(); i++) {
                if (meta.campos[i].nombre.compare(campo, Qt::CaseInsensitive) == 0) {
                    idxCampo = i;
                    break;
                }
            }

            QString valor = (idxCampo != -1 && idxCampo < values.size()) ? values[idxCampo] : "";
            tablaResultados->setItem(row, c, new QTableWidgetItem(valor));
        }
        row++;
    }

    file.close();
}


