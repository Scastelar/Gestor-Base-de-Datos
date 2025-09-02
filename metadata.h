#pragma once
#include <QString>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <QDir>

struct Campo {
    QString nombre;
    QString tipo;
};

class Metadata {
public:
    QString nombreTabla;
    QVector<Campo> campos;

    Metadata(const QString &nombre = "") : nombreTabla(nombre) {}

    void guardar() const {
        // Carpeta donde estarán las tablas
        QDir dir(QDir::currentPath() + "/tables");
        if (!dir.exists()) {
            dir.mkpath("."); // crea la carpeta si no existe
        }

        QFile file(dir.filePath(nombreTabla + ".meta"));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

        QTextStream out(&file);
        out << "Tabla: " << nombreTabla << "\n";
        out << campos.size() << " Campos\n";
        for (const Campo &c : campos) {
            out << c.nombre << " " << c.tipo << "\n";
        }
        file.close();
    }

    static Metadata cargar(const QString &filePath) {
        Metadata meta;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return meta;

        QTextStream in(&file);
        QString etiqueta;
        in >> etiqueta >> meta.nombreTabla;

        int numCampos;
        QString temp;
        in >> numCampos >> temp;

        for (int i = 0; i < numCampos; i++) {
            Campo c;
            in >> c.nombre >> c.tipo;
            meta.campos.append(c);
        }

        file.close();
        return meta;
    }
};
