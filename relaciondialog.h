#ifndef RELACIONDIALOG_H
#define RELACIONDIALOG_H

#include <QDialog>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>

class RelacionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RelacionDialog(const QString &tabla1,
                            const QString &campo1,
                            const QString &tabla2,
                            const QString &campo2,
                            const QString &tipoRelacion,
                            QWidget *parent = nullptr);

    bool integridadReferencial() const { return chkIntegridad->isChecked(); }
    bool cascadaActualizar() const { return chkActualizar->isChecked(); }
    bool cascadaEliminar() const { return chkEliminar->isChecked(); }

private slots:
    void actualizarEstadoCascadas();

private:
    QCheckBox *chkIntegridad;
    QCheckBox *chkActualizar;
    QCheckBox *chkEliminar;
    QPushButton *btnCrear;
    QPushButton *btnCancelar;
};

#endif // RELACIONDIALOG_H
