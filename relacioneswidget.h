#ifndef RELACIONESWIDGET_H
#define RELACIONESWIDGET_H

#include <QWidget>
#include <QTableWidget>
#include <QLabel>
#include <QVBoxLayout>
class RelacionesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit RelacionesWidget(QWidget *parent = nullptr);

signals:
    void cerrada();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
private:
    void crearToolbar();
    void crearLayoutPrincipal();
    void crearPanelTablas();
    void crearAreaRelaciones();
    void crearCardTabla(const QString &nombreTabla); // Función para crear cards de tablas

    QTableWidget *tablaRelaciones;
    QWidget *areaCards; // Área donde se mostrarán las cards de tablas
    QVBoxLayout *cardsLayout; // Layout para organizar las cards
};

#endif // RELACIONESWIDGET_H
