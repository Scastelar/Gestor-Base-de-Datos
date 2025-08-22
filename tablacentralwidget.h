// TablaCentralWidget.h
#ifndef TABLACENTRALWIDGET_H
#define TABLACENTRALWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QIcon>

class TablaCentralWidget : public QWidget {
    Q_OBJECT

public:
    explicit TablaCentralWidget(QWidget *parent = nullptr);

private:
    QTableWidget *tablaCampos;
    QTableWidget *tablaPropiedades;

    void configurarTablaCampos();
    void configurarTablaPropiedades();
    void agregarCampo();
};

#endif // TABLACENTRALWIDGET_H
