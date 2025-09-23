#ifndef FORMULARIOWIDGET_H
#define FORMULARIOWIDGET_H

#include <QWidget>
#include <QFormLayout>
#include <QPushButton>
#include <QMap>
#include <QVariant>
#include <QLabel>
#include <QComboBox>
#include <QLineEdit>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QDateEdit>
#include "metadata.h"

class VistaDatos;              //  Forward declaration
class ValidadorRelaciones;     //  Forward declaration

class FormularioWidget : public QWidget
{
    Q_OBJECT
public:
    void reconstruirFormulario();
    void limpiarFormulario();
    bool camposCambiaron(const QVector<Campo>& nuevosCampos);
    explicit FormularioWidget(const Metadata &meta, VistaDatos *vista, QWidget *parent = nullptr);
    QString getNombreTabla() const { return metadata.nombreTabla; }
    void actualizarDesdeMetadata(const Metadata& nuevaMeta);
private slots:
    void onAgregarClicked();

signals:
    void registroInsertado(const QString& nombreTabla);

private:
    QFormLayout *formLayout;
    QPushButton *btnAgregar;
    QMap<QString, QWidget*> editores;
    Metadata metadata;
    VistaDatos *vistaDatos;
    ValidadorRelaciones *validador;

    void construirFormulario();
    QVariantMap obtenerDatosFormulario();
    QWidget* crearEditorParaCampo(const Campo &c);
};

#endif // FORMULARIOWIDGET_H
