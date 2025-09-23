#pragma once
#include <QString>
#include <QVector>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QSet>
#include <QVariant>
#include <QMap>
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
            return propiedad.isValid() ? propiedad : "Lempira";
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
            if (moneda == "Lempira" || moneda == "Dólar" ||
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

struct Relacion {
    QString tablaPrincipal; // Tabla a la que pertenece la clave primaria
    QString campoPrincipal; // Campo PK
    QString tablaForanea;   // Tabla que tiene la clave foránea
    QString campoForaneo;   // Campo FK
    QString tipoRelacion;   // "1:1", "1:M", "M:M"
};

class Metadata {
public:
    QString nombreTabla;
    QVector<Campo> campos;
    QVector<QMap<QString, QVariant>> registros; //  ahora también almacena registros
    QVector<Relacion> relaciones;

    Metadata(const QString &nombre = "") : nombreTabla(nombre) {}

    bool validarPK() const {
        int countPK = 0;
        for (const Campo &c : campos) {
            if (c.esPK) countPK++;
        }
        return countPK <= 1; // 0 o 1 PK permitida
    }

    bool tienePK() const {
        for (const Campo &c : campos) {
            if (c.esPK) return true;
        }
        return false;
    }

    QString obtenerNombrePK() const {
        for (const Campo &c : campos) {
            if (c.esPK) return c.nombre;
        }
        return "";
    }

    void guardar() const {
        // Validar PK antes de guardar (ahora permite 0 o 1)
        if (!validarPK()) {
            throw std::runtime_error("Solo se permite máximo una clave primaria (PK) por tabla");
        }

        QDir dir(QDir::currentPath() + "/tables");
        if (!dir.exists()) {
            dir.mkpath(".");
        }

        // Guardar definición (.meta)
        QFile file(dir.filePath(nombreTabla + ".meta"));
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("No se pudo abrir el archivo de metadatos para escritura");
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

        // Guardar registros (.data)
        QFile dataFile(dir.filePath(nombreTabla + ".data"));
        if (!dataFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            throw std::runtime_error("No se pudo abrir el archivo de datos para escritura");
        }

        QTextStream outData(&dataFile);
        for (const auto &registro : registros) {
            QStringList valores;
            for (const Campo &c : campos) {
                QVariant valor = registro.value(c.nombre);

                if (c.tipo == "FECHA" && valor.canConvert<QDateTime>()) {
                    valores << valor.toDateTime().toString(Qt::ISODate);
                } else {
                    valores << valor.toString();
                }
            }
            outData << valores.join("|") << "\n";
        }
        dataFile.close();
    }

    static Metadata cargar(const QString &filePath) {
        Metadata meta;

        //  Leer archivo .meta
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            throw std::runtime_error("No se pudo abrir el archivo de metadatos para lectura");
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
            else if (c.tipo == "NUMERO") {
                QString propiedadStr;
                in >> propiedadStr;
                // Validar que la propiedad sea válida para NUMERO
                if (propiedadStr == "entero" || propiedadStr == "decimal" ||
                    propiedadStr == "byte" || propiedadStr == "doble") {
                    c.propiedad = propiedadStr;
                } else {
                    c.propiedad = "entero";  // Valor por defecto
                }
            }
            else if (c.tipo == "MONEDA" || c.tipo == "FECHA") {
                QString propiedadStr;
                in >> propiedadStr;
                c.propiedad = propiedadStr;
            }
            meta.campos.append(c);
        }
        file.close();

        //  Leer archivo .data
        QDir dir(QDir::currentPath() + "/tables");
        QFile dataFile(dir.filePath(meta.nombreTabla + ".data"));
        if (dataFile.exists()) {
            if (!dataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                throw std::runtime_error("No se pudo abrir el archivo de datos para lectura");
            }

            QTextStream inData(&dataFile);
            while (!inData.atEnd()) {
                QString line = inData.readLine().trimmed();
                if (line.isEmpty()) continue;

                QStringList valores = line.split("|");
                if (valores.size() != meta.campos.size()) continue;

                QMap<QString, QVariant> registro;
                for (int i = 0; i < meta.campos.size(); i++) {
                    const Campo &campo = meta.campos[i];
                    QString valorStr = valores[i];

                    // Convertir string al tipo de dato correcto
                    if (campo.tipo == "NUMERO" || campo.tipo == "MONEDA") {
                        bool ok;
                        QString valorLimpio = valorStr.trimmed();

                        if (valorLimpio.isEmpty()) {
                            registro[campo.nombre] = 0.0;  // Valor por defecto para campos vacíos
                        } else {
                            double valorNum = valorLimpio.toDouble(&ok);
                            if (ok) {
                                registro[campo.nombre] = valorNum;
                            } else {
                                // Log del error pero continuar
                                qDebug() << "Error convirtiendo valor numérico:" << valorStr << "para campo" << campo.nombre;
                                registro[campo.nombre] = 0.0;
                            }
                        }
                    }
                    else if (campo.tipo == "FECHA") {
                        // Intentar parsear como fecha ISO
                        QDateTime fecha = QDateTime::fromString(valorStr, Qt::ISODate);
                        if (fecha.isValid()) {
                            registro[campo.nombre] = fecha;
                        } else {
                            // Fallback: usar fecha actual
                            registro[campo.nombre] = QDateTime::currentDateTime();
                        }
                    }
                    else {
                        registro[campo.nombre] = valorStr;
                    }
                }
                meta.registros.append(registro);
            }
            dataFile.close();
        }

        return meta;
    }

    // Métodos auxiliares para propiedades
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


