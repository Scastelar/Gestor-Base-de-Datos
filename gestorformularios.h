#ifndef GESTORFORMULARIOS_H
#define GESTORFORMULARIOS_H

#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include "metadata.h"

class GestorFormularios : public QObject
{
    Q_OBJECT
public:
    explicit GestorFormularios(QObject *parent = nullptr);

    void guardarFormulario(const QString &nombre, const Metadata &meta);
    QJsonArray cargarFormularios();

private:
    QString archivoFormularios;
};

#endif // GESTORFORMULARIOS_H
