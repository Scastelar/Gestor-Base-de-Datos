#ifndef REPORTEWIDGET_H
#define REPORTEWIDGET_H

#include <QDialog>
#include <QTextEdit>
#include <QTableWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include "metadata.h"

class ReporteWidget : public QDialog {
    Q_OBJECT
public:
    explicit ReporteWidget(QWidget *parent = nullptr);

    void generarReporte(const QVector<Metadata> &metadatos);

private:
    QTextEdit *resumen;
    QTableWidget *detalle;
    QPushButton *btnCopiar;
    QPushButton *btnCerrar;

    QString generarTextoResumen(const QVector<Metadata> &metadatos);
};

#endif // REPORTEWIDGET_H
