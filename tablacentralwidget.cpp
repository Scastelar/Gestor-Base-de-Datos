#include "tablacentralwidget.h"
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QPushButton>
#include <QComboBox>

TablaCentralWidget::TablaCentralWidget(QWidget *parent)
    : QWidget(parent)
{
    QVBoxLayout *layoutPrincipal = new QVBoxLayout(this);

    // Crear tabla
    tablaCampos = new QTableWidget(1,3,this);
    configurarTablaCampos();
    tablaCampos->setColumnCount(3); // PK, Nombre, Tipo de dato
    QStringList headers = {"PK", "Nombre del Campo", "Tipo de Dato"};
    tablaCampos->setHorizontalHeaderLabels(headers);

    // Ajustes de estilo
    tablaCampos->horizontalHeader()->setStretchLastSection(true);
    tablaCampos->verticalHeader()->setVisible(false);
    tablaCampos->setSelectionBehavior(QAbstractItemView::SelectRows);
    tablaCampos->setSelectionMode(QAbstractItemView::SingleSelection);
    tablaCampos->setAlternatingRowColors(true);

    // 🔹 Botón para agregar campo
    QPushButton *btnAgregar = new QPushButton("Agregar campo");
    connect(btnAgregar, &QPushButton::clicked, this, &TablaCentralWidget::agregarCampo);

    tablaPropiedades = new QTableWidget(1, 2, this); // una fila inicial, 2 columnas
    configurarTablaPropiedades();

    layoutPrincipal->addWidget(tablaCampos);
    layoutPrincipal->addWidget(tablaPropiedades);
    layoutPrincipal->addWidget(btnAgregar);

}

void TablaCentralWidget::configurarTablaCampos() {
    tablaCampos->setStyleSheet(
        "QTableWidget::item:selected { "
        "background-color: #f0f0f0; "
        "color: black; }"
        );


    QStringList headers;
    headers << "PK" << "Nombre de Campo" << "Tipo de Dato";
    tablaCampos->setHorizontalHeaderLabels(headers);

    // Columna PK con ancho fijo
    tablaCampos->setColumnWidth(0, 50);

    // Insertar icono de llave en primera celda
    QTableWidgetItem *pkItem = new QTableWidgetItem;
    pkItem->setFlags(pkItem->flags() | Qt::ItemIsUserCheckable);
    pkItem->setCheckState(Qt::Unchecked);
    pkItem->setIcon(QIcon(":/imgs/key.png")); // tu icono de llave
    tablaCampos->setItem(0, 0, pkItem);

    // Columna Field Name (editable QTableWidgetItem)
    QTableWidgetItem *nombreItem = new QTableWidgetItem();
    nombreItem->setText("Nuevo Campo");
    tablaCampos->setItem(0, 1, nombreItem);

    // Columna Data Type (QComboBox en la celda)
    QComboBox *tipoCombo = new QComboBox();
    tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
    tablaCampos->setCellWidget(0, 2, tipoCombo);

    tablaCampos->horizontalHeader()->setStretchLastSection(true);
}

void TablaCentralWidget::configurarTablaPropiedades() {
    QStringList headers;
    headers << "Propiedad" << "Valor";
    tablaPropiedades->setHorizontalHeaderLabels(headers);

    // Ejemplo: Tamaño de texto
    tablaPropiedades->setItem(0, 0, new QTableWidgetItem("Tamaño"));
    tablaPropiedades->setItem(0, 1, new QTableWidgetItem("50"));

    tablaPropiedades->horizontalHeader()->setStretchLastSection(true);
}

void TablaCentralWidget::agregarCampo() {
    int row = tablaCampos->rowCount();
    tablaCampos->insertRow(row);

    // Columna PK (checkbox o icono de llave)
    QTableWidgetItem *pkItem = new QTableWidgetItem;
    pkItem->setFlags(pkItem->flags() | Qt::ItemIsUserCheckable);
    pkItem->setCheckState(Qt::Unchecked);
    pkItem->setIcon(QIcon(":/imgs/key.png")); // tu icono de llave
    tablaCampos->setItem(row, 0, pkItem);


    // Columna Field Name (editable QTableWidgetItem)
    QTableWidgetItem *nombreItem = new QTableWidgetItem();
    tablaCampos->setItem(row, 1, nombreItem);

    // Columna Data Type (QComboBox en la celda)
    QComboBox *tipoCombo = new QComboBox();
    tipoCombo->addItems({"TEXTO", "NUMERO", "FECHA", "MONEDA"});
    tablaCampos->setCellWidget(row, 2, tipoCombo);
}
