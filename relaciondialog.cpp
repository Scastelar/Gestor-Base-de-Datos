#include "relaciondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>

RelacionDialog::RelacionDialog(const QString &tabla1,
                               const QString &campo1,
                               const QString &tabla2,
                               const QString &campo2,
                               const QString &tipoRelacion,
                               QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Modificar relaciones");
    setFixedSize(400, 250);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Campos seleccionados
    QGroupBox *grupoCampos = new QGroupBox("Campos relacionados");
    QHBoxLayout *camposLayout = new QHBoxLayout(grupoCampos);

    QLabel *lbl1 = new QLabel(tabla1 + "." + campo1);
    QLabel *lbl2 = new QLabel(tabla2 + "." + campo2);
    camposLayout->addWidget(lbl1);
    camposLayout->addStretch();
    camposLayout->addWidget(lbl2);

    mainLayout->addWidget(grupoCampos);

    // Opciones de integridad
    chkIntegridad = new QCheckBox("Exigir integridad referencial");
    chkActualizar = new QCheckBox("Actualizar en cascada los campos relacionados");
    chkEliminar   = new QCheckBox("Eliminar en cascada los registros relacionados");

    chkActualizar->setEnabled(false);
    chkEliminar->setEnabled(false);

    connect(chkIntegridad, &QCheckBox::toggled, this, &RelacionDialog::actualizarEstadoCascadas);

    mainLayout->addWidget(chkIntegridad);
    mainLayout->addWidget(chkActualizar);
    mainLayout->addWidget(chkEliminar);

    // Tipo de relación
    QLabel *lblTipo = new QLabel("Tipo de relación: " + tipoRelacion);
    mainLayout->addWidget(lblTipo);

    // Botones
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnCrear = new QPushButton("Crear");
    btnCancelar = new QPushButton("Cancelar");

    connect(btnCrear, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancelar, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addStretch();
    btnLayout->addWidget(btnCrear);
    btnLayout->addWidget(btnCancelar);

    mainLayout->addLayout(btnLayout);
}

void RelacionDialog::actualizarEstadoCascadas()
{
    bool enabled = chkIntegridad->isChecked();
    chkActualizar->setEnabled(enabled);
    chkEliminar->setEnabled(enabled);
}
