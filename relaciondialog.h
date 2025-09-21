#ifndef RELACIONDIALOG_H
#define RELACIONDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

class RelacionDialog : public QDialog
{
    Q_OBJECT

public:
    RelacionDialog(const QString &tabla1, const QString &campo1,
                   const QString &tabla2, const QString &campo2,
                   bool esOrigenPK, bool esDestinoPK,
                   QWidget *parent = nullptr);

    QString getTipoRelacion() const;
    void setTipoRelacion(const QString &tipo);
private slots:
    void validarRelacion(int index);

private:
    void actualizarEstadoBoton();

    QComboBox *cmbTipoRelacion;
    QPushButton *btnCrear;

    bool esOrigenPK;
    bool esDestinoPK;
    QString campoSource;
    QString campoDest;
};

#endif // RELACIONDIALOG_H
