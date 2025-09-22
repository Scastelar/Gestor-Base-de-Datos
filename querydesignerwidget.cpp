#include "querydesignerwidget.h"
#include <QHeaderView>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>

QueryDesignerWidget::QueryDesignerWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    grid = new QTableWidget(0, 5, this);
    grid->setHorizontalHeaderLabels({"Campo", "Tabla", "Orden", "Mostrar", "Criterios"});
    grid->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(grid);

    btnEjecutar = new QPushButton("▶ Ejecutar Consulta");
    layout->addWidget(btnEjecutar);

    connect(btnEjecutar, &QPushButton::clicked, this, &QueryDesignerWidget::onEjecutarClicked);
}

void QueryDesignerWidget::agregarCampo(const QString &tabla, const QString &campo)
{
    int row = grid->rowCount();
    grid->insertRow(row);

    grid->setItem(row, 0, new QTableWidgetItem(campo));
    grid->setItem(row, 1, new QTableWidgetItem(tabla));

    // 🔹 Columna Orden
    QComboBox *comboOrden = new QComboBox();
    comboOrden->addItems({"", "Ascendente", "Descendente"});
    grid->setCellWidget(row, 2, comboOrden);

    // 🔹 Columna Mostrar
    QWidget *checkWidget = new QWidget();
    QCheckBox *chk = new QCheckBox();
    QHBoxLayout *h = new QHBoxLayout(checkWidget);
    h->addWidget(chk);
    h->setAlignment(Qt::AlignCenter);
    h->setContentsMargins(0,0,0,0);
    checkWidget->setLayout(h);
    chk->setChecked(true);
    grid->setCellWidget(row, 3, checkWidget);

    // 🔹 Columna Criterios (operador + valor)
    QWidget *criterioWidget = new QWidget();
    QHBoxLayout *hLayout = new QHBoxLayout(criterioWidget);
    hLayout->setContentsMargins(0, 0, 0, 0);

    QComboBox *comboOperador = new QComboBox();
    comboOperador->addItems({"", "=", ">", ">=", "<", "<="}); // "" = sin criterio

    QLineEdit *editValor = new QLineEdit();
    editValor->setPlaceholderText("Valor");

    hLayout->addWidget(comboOperador);
    hLayout->addWidget(editValor);
    criterioWidget->setLayout(hLayout);

    grid->setCellWidget(row, 4, criterioWidget);
}


void QueryDesignerWidget::onEjecutarClicked()
{
    QString sql = generarSQL();
    emit ejecutarConsulta(sql);
}

QString QueryDesignerWidget::generarSQL() const
{
    QStringList campos;
    QString tabla = "";
    QString criterioWhere = "";

    for (int row = 0; row < grid->rowCount(); ++row) {
        QString campo = grid->item(row, 0)->text();
        QString tablaRow = grid->item(row, 1)->text();

        if (tabla.isEmpty()) tabla = tablaRow;

        // 🔹 Mostrar
        QWidget *w = grid->cellWidget(row, 3);
        QCheckBox *chk = w->findChild<QCheckBox*>();
        if (chk && chk->isChecked()) {
            campos << campo;
        }

        // 🔹 Criterios
        QWidget *critWidget = grid->cellWidget(row, 4);
        if (critWidget) {
            QComboBox *combo = critWidget->findChild<QComboBox*>();
            QLineEdit *edit = critWidget->findChild<QLineEdit*>();

            if (combo && edit) {
                QString op = combo->currentText();
                QString val = edit->text();

                if (!op.isEmpty() && !val.isEmpty()) {
                    if (criterioWhere.isEmpty()) {
                        criterioWhere = QString(" WHERE %1 %2 '%3'")
                        .arg(campo, op, val);
                    } else {
                        criterioWhere += QString(" AND %1 %2 '%3'")
                        .arg(campo, op, val);
                    }
                }
            }
        }
    }

    if (campos.isEmpty()) campos << "*";
    return QString("SELECT %1 FROM %2%3")
        .arg(campos.join(", "))
        .arg(tabla)
        .arg(criterioWhere);
}

