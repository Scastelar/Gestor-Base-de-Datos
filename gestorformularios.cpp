#include "gestorformularios.h"
#include <QDir>
#include <QDebug>

GestorFormularios::GestorFormularios(QObject *parent)
    : QObject(parent)
{
    archivoFormularios = QDir::currentPath() + "/formularios.json";
}

void GestorFormularios::guardarFormulario(const QString &nombre, const Metadata &meta)
{
    QJsonArray formularios = cargarFormularios();

    QJsonObject nuevoFormulario;
    nuevoFormulario["nombre"] = nombre;
    nuevoFormulario["tablaOrigen"] = meta.nombreTabla;

    QJsonArray camposArr;
    for (const Campo &c : meta.campos) {
        camposArr.append(c.nombre);
    }
    nuevoFormulario["campos"] = camposArr;

    formularios.append(nuevoFormulario);

    QJsonObject root;
    root["formularios"] = formularios;

    QFile file(archivoFormularios);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        file.close();
    } else {
        qDebug() << "❌ Error al guardar formulario en" << archivoFormularios;
    }
}

QJsonArray GestorFormularios::cargarFormularios()
{
    QFile file(archivoFormularios);
    if (!file.exists()) return QJsonArray();

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isObject()) {
            return doc.object()["formularios"].toArray();
        }
    }
    return QJsonArray();
}
