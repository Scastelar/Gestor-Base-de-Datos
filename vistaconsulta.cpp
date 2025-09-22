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

    // Separar SELECT y FROM
    QString selectPart = sql.mid(6, idxFrom - 6).trimmed();
    QString fromPart   = sql.mid(idxFrom + 6).trimmed();

    // Criterios (si existen)
    QString wherePart = "";
    int idxWhere = fromPart.toUpper().indexOf(" WHERE ");
    if (idxWhere != -1) {
        wherePart = fromPart.mid(idxWhere + 7).trimmed();
        fromPart = fromPart.left(idxWhere).trimmed();
    }

    // Campos
    QStringList campos = selectPart.split(",", Qt::SkipEmptyParts);
    for (QString &c : campos) c = c.trimmed();

    // Nombre de tabla
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

    // Limpiar tabla
    tablaResultados->clearContents();
    tablaResultados->setRowCount(0);
    tablaResultados->setColumnCount(campos.size());
    tablaResultados->setHorizontalHeaderLabels(campos);

    // Preparar criterios
    QString criterioCampo, criterioOperador, criterioValor;
    if (!wherePart.isEmpty()) {
        QStringList parts = wherePart.split(" ", Qt::SkipEmptyParts);
        if (parts.size() >= 3) {
            criterioCampo = parts[0];
            criterioOperador = parts[1];
            criterioValor = parts[2];
            criterioValor.remove("'"); // quitar comillas
        }
    }

    // Mostrar resultados
    int row = 0;
    for (const auto &registro : meta.registros) {
        bool coincide = true;

        if (!criterioCampo.isEmpty()) {
            QVariant valor = registro.value(criterioCampo);
            QString valorStr = valor.toString();

            if (criterioOperador == "=") {
                coincide = (valorStr == criterioValor);
            } else if (criterioOperador == ">") {
                coincide = valor.toDouble() > criterioValor.toDouble();
            } else if (criterioOperador == ">=") {
                coincide = valor.toDouble() >= criterioValor.toDouble();
            } else if (criterioOperador == "<") {
                coincide = valor.toDouble() < criterioValor.toDouble();
            } else if (criterioOperador == "<=") {
                coincide = valor.toDouble() <= criterioValor.toDouble();
            }
        }

        if (!coincide) continue;

        tablaResultados->insertRow(row);
        for (int c = 0; c < campos.size(); c++) {
            QString campo = campos[c];
            QVariant valor;

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




