#ifndef QUERYDESIGNERWIDGET_H
#define QUERYDESIGNERWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include "metadata.h"

class QueryDesignerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit QueryDesignerWidget(QWidget *parent = nullptr);

signals:
    void ejecutarConsulta(const QString &sqlLike);

public slots:
    void agregarCampo(const QString &tabla, const QString &campo);

private slots:
    void onEjecutarClicked();

private:
    QTableWidget *grid;
    QPushButton *btnEjecutar;

    QString generarSQL() const;
};

#endif // QUERYDESIGNERWIDGET_H
