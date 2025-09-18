#include "querydesignerwidget.h"
#include <QHeaderView>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>

QueryDesignerWidget::QueryDesignerWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);

    // Grid de diseño de consulta
    grid = new QTableWidget(0, 5, this);
    grid->setHorizontalHeaderLabels({"Campo", "Tabla", "Orden", "Mostrar", "Criterios"});
    grid->horizontalHeader()->setStretchLastSection(true);
    layout->addWidget(grid);

    // Botón ejecutar
    btnEjecutar = new QPushButton("▶ Ejecutar Consulta");
    layout->addWidget(btnEjecutar);

    connect(btnEjecutar, &QPushButton::clicked, this, &QueryDesignerWidget::onEjecutarClicked);
}

void QueryDesignerWidget::agregarCampo(const QString &tabla, const QString &campo)
{
    int row = grid->rowCount();
    grid->insertRow(row);

    // Campo
    grid->setItem(row, 0, new QTableWidgetItem(campo));

    // Tabla
    grid->setItem(row, 1, new QTableWidgetItem(tabla));

    // Orden (ComboBox)
    QComboBox *comboOrden = new QComboBox();
    comboOrden->addItems({"", "Ascendente", "Descendente"});
    grid->setCellWidget(row, 2, comboOrden);

    // Mostrar (CheckBox)
    QWidget *checkWidget = new QWidget();
    QCheckBox *chk = new QCheckBox();
    QHBoxLayout *h = new QHBoxLayout(checkWidget);
    h->addWidget(chk);
    h->setAlignment(Qt::AlignCenter);
    h->setContentsMargins(0,0,0,0);
    checkWidget->setLayout(h);
    chk->setChecked(true);
    grid->setCellWidget(row, 3, checkWidget);

    // Criterios
    QLineEdit *criterio = new QLineEdit();
    grid->setCellWidget(row, 4, criterio);
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

    for (int row = 0; row < grid->rowCount(); ++row) {
        QString campo = grid->item(row, 0)->text();
        QString tablaRow = grid->item(row, 1)->text();

        if (tabla.isEmpty()) tabla = tablaRow;

        // Mostrar ?
        QWidget *w = grid->cellWidget(row, 3);
        QCheckBox *chk = w->findChild<QCheckBox*>();
        if (chk && chk->isChecked()) {
            campos << campo;
        }
    }

    if (campos.isEmpty()) campos << "*";
    return QString("SELECT %1 FROM %2").arg(campos.join(", ")).arg(tabla);
}
