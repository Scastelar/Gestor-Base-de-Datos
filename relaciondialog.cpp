#include "relaciondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

RelacionDialog::RelacionDialog(const QString &tabla1,
                               const QString &campo1,
                               const QString &tabla2,
                               const QString &campo2,
                               bool esOrigenPK,
                               bool esDestinoPK,
                               QWidget *parent)
    : QDialog(parent), esOrigenPK(esOrigenPK), esDestinoPK(esDestinoPK)
{
    setWindowTitle("Crear Relación");
    setFixedSize(400, 200);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Campos seleccionados
    QGroupBox *grupoCampos = new QGroupBox("Campos relacionados");
    QVBoxLayout *camposLayout = new QVBoxLayout(grupoCampos);

    QLabel *lbl1 = new QLabel("Origen: " + tabla1 + "." + campo1 +
                              (esOrigenPK ? " (PK)" : " (FK)"));
    QLabel *lbl2 = new QLabel("Destino: " + tabla2 + "." + campo2 +
                              (esDestinoPK ? " (PK)" : " (FK)"));

    camposLayout->addWidget(lbl1);
    camposLayout->addWidget(lbl2);

    mainLayout->addWidget(grupoCampos);

    // Tipo de relación
    QLabel *lblTipo = new QLabel("Tipo de relación:");
    mainLayout->addWidget(lblTipo);

    cmbTipoRelacion = new QComboBox(this);
    cmbTipoRelacion->addItem("Uno a Uno", "1:1");
    cmbTipoRelacion->addItem("Uno a Muchos", "1:M");
    cmbTipoRelacion->addItem("Muchos a Muchos", "M:M");
    mainLayout->addWidget(cmbTipoRelacion);

    // Conectar el combo box a la función de validación
    connect(cmbTipoRelacion, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &RelacionDialog::validarRelacion);

    // Botones
    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnCrear = new QPushButton("Crear");
    QPushButton *btnCancelar = new QPushButton("Cancelar");

    connect(btnCrear, &QPushButton::clicked, this, &QDialog::accept);
    connect(btnCancelar, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addStretch();
    btnLayout->addWidget(btnCrear);
    btnLayout->addWidget(btnCancelar);

    mainLayout->addLayout(btnLayout);

    // Validación inicial
    validarRelacion(0);
}

void RelacionDialog::validarRelacion(int index)
{
    QString tipoSeleccionado = cmbTipoRelacion->currentData().toString();
    bool valido = false;

    if (tipoSeleccionado == "1:1") {
        // Validación: ambos deben ser PKs
        valido = (esOrigenPK && esDestinoPK);
        if (!valido) {
            QMessageBox::warning(this, "Relación inválida",
                                 "Para una relación Uno a Uno, ambos campos deben ser Claves Primarias (PK).");
        }
    }
    else if (tipoSeleccionado == "1:M") {
        // Validación: uno es PK y el otro es FK (o no es PK)
        valido = (esOrigenPK && !esDestinoPK) || (!esOrigenPK && esDestinoPK);
        if (!valido) {
            QMessageBox::warning(this, "Relación inválida",
                                 "Para una relación Uno a Muchos, un campo debe ser Clave Primaria (PK) y el otro debe ser Clave Foránea (FK).");
        }
    }
    else if (tipoSeleccionado == "M:M") {
        // Validación: ambos son FKs (o no son PKs)
        valido = (!esOrigenPK && !esDestinoPK);
        if (!valido) {
            QMessageBox::warning(this, "Relación inválida",
                                 "Para una relación Muchos a Muchos, ambos campos deben ser Claves Foráneas (FK).");
        }
    }

    btnCrear->setEnabled(valido);
}

QString RelacionDialog::getTipoRelacion() const
{
    return cmbTipoRelacion->currentData().toString();
}
