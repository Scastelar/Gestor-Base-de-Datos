#include "relaciondialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QCheckBox>
RelacionDialog::RelacionDialog(const QString &tabla1,
                               const QString &campo1,
                               const QString &tabla2,
                               const QString &campo2,
                               bool esOrigenPK,
                               bool esDestinoPK,
                               QWidget *parent)
    : QDialog(parent),
    esOrigenPK(esOrigenPK),
    esDestinoPK(esDestinoPK),
    campoSource(campo1),   //  Guardar el nombre del campo origen
    campoDest(campo2)
{
    setWindowTitle("Crear Relación");
    setMinimumSize(450, 300); // asegura tamaño mínimo cómodo
    resize(sizeHint());       // ajusta según contenido


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

    // Opciones de integridad referencial (solo visuales)
    QGroupBox *grupoIntegridad = new QGroupBox(this);
    QVBoxLayout *integridadLayout = new QVBoxLayout(grupoIntegridad);

    QCheckBox *chkIntegridad = new QCheckBox("Exigir integridad referencial", grupoIntegridad);
    QCheckBox *chkActualizar = new QCheckBox("Actualizar en cascada los campos relacionados", grupoIntegridad);
    QCheckBox *chkEliminar = new QCheckBox("Eliminar en cascada los registros relacionados", grupoIntegridad);

    // Por defecto deshabilitadas como en Access
    chkActualizar->setEnabled(false);
    chkEliminar->setEnabled(false);

    //  Habilitar/Deshabilitar las otras dos según integridad
    connect(chkIntegridad, &QCheckBox::toggled, this, [chkActualizar, chkEliminar](bool checked) {
        chkActualizar->setEnabled(checked);
        chkEliminar->setEnabled(checked);
    });


    integridadLayout->addWidget(chkIntegridad);
    integridadLayout->addWidget(chkActualizar);
    integridadLayout->addWidget(chkEliminar);

    grupoIntegridad->setLayout(integridadLayout);
    mainLayout->addWidget(grupoIntegridad);
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

}

void RelacionDialog::validarRelacion(int index)
{
    QString tipoSeleccionado = cmbTipoRelacion->currentData().toString();
    bool valido = false;

    //  Nueva validación: nombres de campo deben coincidir (case-insensitive)
    if (campoSource.compare(campoDest, Qt::CaseInsensitive) != 0) {
        QMessageBox::warning(this, "Relación inválida",
                             "Los nombres de los campos deben coincidir exactamente (ignorando mayúsculas/minúsculas).");
        btnCrear->setEnabled(false);
        return;
    }

    if (tipoSeleccionado == "1:1") {
        valido = (esOrigenPK && esDestinoPK);
        if (!valido) {
            QMessageBox::warning(this, "Relación inválida",
                                 "Para una relación Uno a Uno, ambos campos deben ser Claves Primarias (PK).");
        }
    }
    else if (tipoSeleccionado == "1:M") {
        valido = (esOrigenPK && !esDestinoPK) || (!esOrigenPK && esDestinoPK);
        if (!valido) {
            QMessageBox::warning(this, "Relación inválida",
                                 "Para una relación Uno a Muchos, un campo debe ser Clave Primaria (PK) y el otro debe ser Clave Foránea (FK).");
        }
    }
    else if (tipoSeleccionado == "M:M") {
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
void RelacionDialog::setTipoRelacion(const QString &tipo)
{
    int index = cmbTipoRelacion->findData(tipo);
    if (index != -1) {
        cmbTipoRelacion->setCurrentIndex(index);
    }
}

