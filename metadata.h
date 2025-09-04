#pragma once
#include <QString>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSet>
#include <QVariant>
#include <stdexcept>

struct Campo {
    QString nombre;
    QString tipo;
    bool esPK;
    QVariant propiedad;

    QVariant obtenerPropiedad() const {
        if (tipo == "TEXTO") {
            return propiedad.isValid() ? propiedad : 255;
        }
        else if (tipo == "NUMERO") {
            return propiedad.isValid() ? propiedad : "entero";
        }
        else if (tipo == "MONEDA") {
            return propiedad.isValid() ? propiedad : "Moneda Lps";
        }
        else if (tipo == "FECHA") {
            return propiedad.isValid() ? propiedad : "DD-MM-YY";
        }
        return QVariant();
    }

    bool establecerPropiedad(const QVariant& nuevaPropiedad) {
        if (tipo == "TEXTO") {
            bool ok;
            int maxCaracteres = nuevaPropiedad.toInt(&ok);
            if (ok && maxCaracteres > 0) {
                propiedad = maxCaracteres;
                return true;
            }
            return false;
        }
        else if (tipo == "NUMERO") {
            QString tipoNumero = nuevaPropiedad.toString().toLower();
            if (tipoNumero == "entero" || tipoNumero == "decimal" ||
                tipoNumero == "byte" || tipoNumero == "doble") {
                propiedad = tipoNumero;
                return true;
            }
            return false;
        }
        else if (tipo == "MONEDA") {
            QString moneda = nuevaPropiedad.toString();
            if (moneda == "Moneda Lps" || moneda == "Dólar" ||
                moneda == "Euros" || moneda == "Millares") {
                propiedad = moneda;
                return true;
            }
            return false;
        }
        else if (tipo == "FECHA") {
            QString formato = nuevaPropiedad.toString();
            if (formato == "DD-MM-YY" || formato == "DD/MM/YY" ||
                formato == "DD/MES/YYYY" || formato == "YYYY-MM-DD") {
                propiedad = formato;
                return true;
            }
            return false;
        }
        return false;
    }
};

class Metadata {
public:
    QString nombreTabla;
    QVector<Campo> campos;

    Metadata(const QString &nombre = "") : nombreTabla(nombre) {}

    bool validarPK() const {
        int countPK = 0;
        for (const Campo &c : campos) {
            if (c.esPK) countPK++;
        }
        return countPK == 1;
    }

    void guardar() {
        // Validar PK antes de guardar
        int countPK = 0;
        for (Campo &c : campos) {
            if (c.esPK) countPK++;
        }

        if (countPK == 0 && !campos.isEmpty()) {
            campos[0].esPK = true; // 🔹 asignar PK automática al primer campo
        }
        else if (countPK > 1) {
            throw std::runtime_error("Solo debe existir una clave primaria (PK)");
        }

        QDir dir(QDir::currentPath() + "/tables");
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        QFile file(dir.filePath(nombreTabla + ".meta"));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("No se pudo abrir el archivo para escritura");
        }

        QTextStream out(&file);
        out << "Tabla: " << nombreTabla << "\n";
        out << campos.size() << " Campos\n";
        for (const Campo &c : campos) {
            out << c.nombre << " " << c.tipo << " " << (c.esPK ? "PK" : "NPK");

            if (c.tipo == "TEXTO") {
                out << " " << c.obtenerPropiedad().toInt();
            }
            else if (c.tipo == "NUMERO" || c.tipo == "MONEDA" || c.tipo == "FECHA") {
                out << " " << c.obtenerPropiedad().toString();
            }
            out << "\n";
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

            if (c.tipo == "TEXTO") {
                int maxCaracteres;
                in >> maxCaracteres;
                c.propiedad = maxCaracteres;
            }
            else if (c.tipo == "NUMERO" || c.tipo == "MONEDA" || c.tipo == "FECHA") {
                QString propiedadStr;
                in >> propiedadStr;
                c.propiedad = propiedadStr;
            }

            meta.campos.append(c);
        }

        file.close();
        return meta;
    }

    static bool eliminarTabla(const QString &nombreTabla) {
        QDir dir(QDir::currentPath() + "/tables");
        if (!dir.exists()) return false;

        QString metaFilePath = dir.filePath(nombreTabla + ".meta");
        QString dataFilePath = dir.filePath(nombreTabla + ".data");
        QString availFilePath = dir.filePath("avail.list");

        if (!QFile::exists(metaFilePath)) return false;

        QFile availFile(availFilePath);
        if (availFile.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&availFile);
            out << nombreTabla << "\n";
            availFile.close();
        }

        bool metaEliminado = QFile::remove(metaFilePath);
        bool dataEliminado = QFile::remove(dataFilePath);

        return metaEliminado && dataEliminado;
    }

    static QSet<QString> obtenerTablasDisponibles() {
        QSet<QString> disponibles;
        QDir dir(QDir::currentPath() + "/tables");

        if (!dir.exists()) return disponibles;

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

    static void limpiarAvailList() {
        QDir dir(QDir::currentPath() + "/tables");
        if (!dir.exists()) return;
        QString availFilePath = dir.filePath("avail.list");
        QFile::remove(availFilePath);
    }

    bool establecerPropiedadCampo(const QString& nombreCampo, const QVariant& propiedad) {
        for (Campo &c : campos) {
            if (c.nombre == nombreCampo) {
                return c.establecerPropiedad(propiedad);
            }
        }
        return false;
    }

    QVariant obtenerPropiedadCampo(const QString& nombreCampo) const {
        for (const Campo &c : campos) {
            if (c.nombre == nombreCampo) {
                return c.obtenerPropiedad();
            }
        }
        return QVariant();
    }
};

