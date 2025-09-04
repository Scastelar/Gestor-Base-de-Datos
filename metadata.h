#pragma once
#include <QString>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSet>

struct Campo {
    QString nombre;
    QString tipo;
    bool esPK;  // Nuevo campo para indicar si es clave primaria
};

class Metadata {
public:
    QString nombreTabla;
    QVector<Campo> campos;

    Metadata(const QString &nombre = "") : nombreTabla(nombre) {}

    // Validar que exista mínimo 1 y máximo 1 PK
    bool validarPK() const {
        int countPK = 0;
        for (const Campo &c : campos) {
            if (c.esPK) {
                countPK++;
            }
        }
        return countPK >= 1 && countPK <= 1;
    }

    void guardar() const {
        // Validar PK antes de guardar
        if (!validarPK()) {
            throw std::runtime_error("Debe existir exactamente un campo como clave primaria (PK)");
        }

        // Carpeta donde estarán las tablas
        QDir dir(QDir::currentPath() + "/tables");
        if (!dir.exists()) {
            dir.mkpath("."); // crea la carpeta si no existe
        }

        QFile file(dir.filePath(nombreTabla + ".meta"));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("No se pudo abrir el archivo para escritura");
        }

        QTextStream out(&file);
        out << "Tabla: " << nombreTabla << "\n";
        out << campos.size() << " Campos\n";
        for (const Campo &c : campos) {
            out << c.nombre << " " << c.tipo << " " << (c.esPK ? "PK" : "NPK") << "\n";
        }
        file.close();
    }

    static Metadata cargar(const QString &filePath) {
        Metadata meta;
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            throw std::runtime_error("No se pudo abrir el archivo para lectura");
        }

        QTextStream in(&file);
        QString etiqueta;
        in >> etiqueta >> meta.nombreTabla;

        int numCampos;
        QString temp;
        in >> numCampos >> temp;

        for (int i = 0; i < numCampos; i++) {
            Campo c;
            QString pkFlag;
            in >> c.nombre >> c.tipo >> pkFlag;
            c.esPK = (pkFlag == "PK");
            meta.campos.append(c);
        }

        file.close();
        return meta;
    }

    // Método para eliminar tabla utilizando AVAIL LIST
    static bool eliminarTabla(const QString &nombreTabla) {
        QDir dir(QDir::currentPath() + "/tables");
        if (!dir.exists()) {
            return false; // Carpeta no existe
        }

        QString metaFilePath = dir.filePath(nombreTabla + ".meta");
        QString dataFilePath = dir.filePath(nombreTabla + ".data");
        QString availFilePath = dir.filePath("avail.list");

        // Verificar que la tabla existe
        if (!QFile::exists(metaFilePath)) {
            return false;
        }

        // Agregar a la lista de disponibles (AVAIL LIST)
        QFile availFile(availFilePath);
        if (availFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&availFile);
            out << nombreTabla << "\n";
            availFile.close();
        }

        // Eliminar archivos de metadatos y datos
        bool metaEliminado = QFile::remove(metaFilePath);
        bool dataEliminado = QFile::remove(dataFilePath);

        return metaEliminado && dataEliminado;
    }

    // Método para obtener la lista de tablas disponibles
    static QSet<QString> obtenerTablasDisponibles() {
        QSet<QString> disponibles;
        QDir dir(QDir::currentPath() + "/tables");

        if (!dir.exists()) {
            return disponibles;
        }

        QString availFilePath = dir.filePath("avail.list");
        QFile availFile(availFilePath);

        if (availFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&availFile);
            while (!in.atEnd()) {
                QString tabla = in.readLine().trimmed();
                if (!tabla.isEmpty()) {
                    disponibles.insert(tabla);
                }
            }
            availFile.close();
        }

        return disponibles;
    }

    // Método para limpiar la lista de disponibles
    static void limpiarAvailList() {
        QDir dir(QDir::currentPath() + "/tables");
        if (!dir.exists()) {
            return;
        }

        QString availFilePath = dir.filePath("avail.list");
        QFile::remove(availFilePath);
    }
};
