#include "formulariowidget.h"
#include "vistadatos.h"
#include "validadorrelaciones.h"
#include <QVBoxLayout>
#include <QGroupBox>
#include <QMessageBox>

FormularioWidget::FormularioWidget(const Metadata &meta, VistaDatos *vista, QWidget *parent)
    : QWidget(parent), metadata(meta), vistaDatos(vista)
{
    validador = vistaDatos->getValidadorRelaciones(); // 🔗 acceso al validador FK

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Título
    QLabel *titulo = new QLabel(metadata.nombreTabla);
    titulo->setStyleSheet("font-size: 22px; font-weight: bold; color: #2b579a; padding: 15px;");
    titulo->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titulo);

    // Contenedor de campos
    QGroupBox *grupo = new QGroupBox("Formulario " + metadata.nombreTabla, this);
    grupo->setStyleSheet("QGroupBox { font-weight: bold; border: 1px solid #cccccc; margin-top: 10px; }");
    QVBoxLayout *groupLayout = new QVBoxLayout(grupo);

    formLayout = new QFormLayout();
    formLayout->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    formLayout->setFormAlignment(Qt::AlignLeft | Qt::AlignTop);
    formLayout->setHorizontalSpacing(25);
    formLayout->setVerticalSpacing(18);

    construirFormulario();
    groupLayout->addLayout(formLayout);

    // Botón de acción
    btnAgregar = new QPushButton("➕ Agregar datos a tabla", this);
    btnAgregar->setStyleSheet(
        "QPushButton { background-color: #2b579a; color: white; padding: 12px 20px; "
        "border-radius: 6px; font-size: 14px; font-weight: bold; } "
        "QPushButton:hover { background-color: #1e3f6e; }");
    groupLayout->addWidget(btnAgregar, 0, Qt::AlignRight);

    mainLayout->addWidget(grupo);

    connect(btnAgregar, &QPushButton::clicked, this, &FormularioWidget::onAgregarClicked);
}

QWidget* FormularioWidget::crearEditorParaCampo(const Campo &c)
{
    QWidget *editor = nullptr;

    // 🔹 Si es clave foránea → ComboBox con valores válidos
    if (validador && validador->esCampoClaveForanea(metadata.nombreTabla, c.nombre)) {
        QComboBox *combo = new QComboBox(this);
        combo->setStyleSheet("QComboBox { font-size: 14px; padding: 6px; }");
        QStringList valores = validador->obtenerValoresValidos(metadata.nombreTabla, c.nombre);
        combo->addItems(valores);
        editor = combo;
    }
    else if (c.tipo == "TEXTO") {
        QLineEdit *line = new QLineEdit(this);
        line->setMaxLength(c.obtenerPropiedad().toInt());
        line->setMinimumHeight(30);
        line->setStyleSheet("QLineEdit { font-size: 14px; padding: 6px; }");
        editor = line;
    }
    else if (c.tipo == "NUMERO") {
        QString subtipo = c.obtenerPropiedad().toString();
        if (subtipo == "entero") {
            QSpinBox *spin = new QSpinBox(this);
            spin->setRange(INT_MIN, INT_MAX);
            spin->setStyleSheet("QSpinBox { font-size: 14px; padding: 6px; }");
            editor = spin;
        } else {
            QDoubleSpinBox *dspin = new QDoubleSpinBox(this);
            dspin->setDecimals(2);
            dspin->setRange(-1e9, 1e9);
            dspin->setStyleSheet("QDoubleSpinBox { font-size: 14px; padding: 6px; }");
            editor = dspin;
        }
    }
    else if (c.tipo == "FECHA") {
        QDateEdit *date = new QDateEdit(QDate::currentDate(), this);
        date->setCalendarPopup(true);
        date->setStyleSheet("QDateEdit { font-size: 14px; padding: 6px; }");
        editor = date;
    }
    else if (c.tipo == "MONEDA") {
        QDoubleSpinBox *money = new QDoubleSpinBox(this);
        money->setDecimals(2);
        money->setRange(-1e9, 1e9);
        money->setPrefix(c.obtenerPropiedad().toString() + " ");
        money->setStyleSheet("QDoubleSpinBox { font-size: 14px; padding: 6px; }");
        editor = money;
    }

    return editor;
}

void FormularioWidget::construirFormulario()
{
    for (const Campo &c : metadata.campos) {
        QWidget *editor = crearEditorParaCampo(c);
        if (editor) {
            editores[c.nombre] = editor;
            QLabel *label = new QLabel(c.nombre + ":");
            label->setStyleSheet("font-size: 14px; font-weight: bold;");
            formLayout->addRow(label, editor);
        }
    }
}

QVariantMap FormularioWidget::obtenerDatosFormulario()
{
    QVariantMap registro;
    for (const Campo &c : metadata.campos) {
        QWidget *w = editores.value(c.nombre, nullptr);
        if (!w) continue;

        if (validador && validador->esCampoClaveForanea(metadata.nombreTabla, c.nombre)) {
            registro[c.nombre] = qobject_cast<QComboBox*>(w)->currentText();
        }
        else if (c.tipo == "TEXTO") {
            registro[c.nombre] = qobject_cast<QLineEdit*>(w)->text();
        }
        else if (c.tipo == "NUMERO") {
            if (auto spin = qobject_cast<QSpinBox*>(w)) registro[c.nombre] = spin->value();
            else if (auto dspin = qobject_cast<QDoubleSpinBox*>(w)) registro[c.nombre] = dspin->value();
        }
        else if (c.tipo == "MONEDA") {
            registro[c.nombre] = qobject_cast<QDoubleSpinBox*>(w)->value();
        }
        else if (c.tipo == "FECHA") {
            registro[c.nombre] = qobject_cast<QDateEdit*>(w)->date();
        }
    }
    return registro;
}

void FormularioWidget::onAgregarClicked()
{
    QVariantMap nuevo = obtenerDatosFormulario();
    if (vistaDatos) {
        vistaDatos->insertarRegistroDesdeFormulario(nuevo);
        QMessageBox::information(this, "Éxito", "Registro agregado a la tabla.");

        // Limpiar formulario después de insertar
        limpiarFormulario();
    }
}
void FormularioWidget::actualizarDesdeMetadata(const Metadata& nuevaMeta) {
    if (nuevaMeta.nombreTabla != metadata.nombreTabla) {
        return; // No es la misma tabla
    }

    // Actualizar metadata
    metadata = nuevaMeta;

    // Reconstruir formulario solo si los campos cambiaron
    if (camposCambiaron(nuevaMeta.campos)) {
        reconstruirFormulario();
    }

    qDebug() << "Formulario actualizado para tabla:" << metadata.nombreTabla;
}

bool FormularioWidget::camposCambiaron(const QVector<Campo>& nuevosCampos) {
    if (nuevosCampos.size() != metadata.campos.size()) {
        return true;
    }

    for (int i = 0; i < nuevosCampos.size(); ++i) {
        if (nuevosCampos[i].nombre != metadata.campos[i].nombre ||
            nuevosCampos[i].tipo != metadata.campos[i].tipo) {
            return true;
        }
    }

    return false;
}

void FormularioWidget::reconstruirFormulario() {
    // Limpiar formulario actual
    while (formLayout->rowCount() > 0) {
        formLayout->removeRow(0);
    }
    editores.clear();

    // Reconstruir con nuevos campos
    construirFormulario();
}

void FormularioWidget::limpiarFormulario() {
    for (auto it = editores.begin(); it != editores.end(); ++it) {
        QWidget* editor = it.value();

        // Verificar si es ComboBox (clave foránea)
        if (QComboBox* combo = qobject_cast<QComboBox*>(editor)) {
            combo->setCurrentIndex(0);
        }
        // Verificar si es QLineEdit (texto)
        else if (QLineEdit* line = qobject_cast<QLineEdit*>(editor)) {
            line->clear();
        }
        // Verificar si es QSpinBox (número entero)
        else if (QSpinBox* spin = qobject_cast<QSpinBox*>(editor)) {
            spin->setValue(0);
        }
        // Verificar si es QDoubleSpinBox (número decimal o moneda)
        else if (QDoubleSpinBox* dspin = qobject_cast<QDoubleSpinBox*>(editor)) {
            dspin->setValue(0.0);
        }
        // Verificar si es QDateEdit (fecha)
        else if (QDateEdit* date = qobject_cast<QDateEdit*>(editor)) {
            date->setDate(QDate::currentDate());
        }
    }
}

