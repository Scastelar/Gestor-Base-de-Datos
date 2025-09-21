#include "validadorrelaciones.h"
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QDebug>

ValidadorRelaciones::ValidadorRelaciones(QObject *parent)
    : QObject(parent)
{
    cargarRelaciones();
}

void ValidadorRelaciones::cargarRelaciones(const QString &archivoRelaciones)
{
    relaciones.clear();
    cacheMetadatos.clear(); // Limpiar cache

    QFile archivo(archivoRelaciones);
    if (!archivo.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "No se pudo abrir el archivo de relaciones:" << archivoRelaciones;
        return;
    }

    QTextStream in(&archivo);
    while (!in.atEnd()) {
        QString linea = in.readLine().trimmed();
        if (linea.isEmpty() || linea.startsWith("#")) continue;

        QStringList partes = linea.split("|");
        if (partes.size() == 4) {
            RelacionFK rel = procesarRelacion(partes);
            if (rel.esValida()) {
                relaciones.append(rel);
                qDebug() << "🔗 Relación FK cargada:" << rel.tablaForanea
                         << "." << rel.campoForaneo << " -> "
                         << rel.tablaPrincipal << "." << rel.campoPrincipal;
            }
        }
    }
    archivo.close();

    qDebug() << "✅ Total de relaciones FK cargadas:" << relaciones.size();
}

RelacionFK ValidadorRelaciones::procesarRelacion(const QStringList &partes)
{
    QString tabla1 = partes[0];
    QString campo1 = partes[1];
    QString tabla2 = partes[2];
    QString campo2 = partes[3];

    // Determinar cuál tabla tiene la PK para establecer la dirección correcta
    Metadata meta1 = obtenerMetadata(tabla1);
    Metadata meta2 = obtenerMetadata(tabla2);

    bool campo1EsPK = false, campo2EsPK = false;

    // Buscar si campo1 es PK en tabla1
    for (const Campo &c : meta1.campos) {
        if (c.nombre == campo1 && c.esPK) {
            campo1EsPK = true;
            break;
        }
    }

    // Buscar si campo2 es PK en tabla2
    for (const Campo &c : meta2.campos) {
        if (c.nombre == campo2 && c.esPK) {
            campo2EsPK = true;
            break;
        }
    }

    RelacionFK rel;

    if (campo1EsPK && !campo2EsPK) {
        // tabla1 es principal, tabla2 es foránea (1:M)
        rel.tablaPrincipal = tabla1;
        rel.campoPrincipal = campo1;
        rel.tablaForanea = tabla2;
        rel.campoForaneo = campo2;
        rel.tipoRelacion = "1:M";
    }
    else if (campo2EsPK && !campo1EsPK) {
        // tabla2 es principal, tabla1 es foránea (1:M)
        rel.tablaPrincipal = tabla2;
        rel.campoPrincipal = campo2;
        rel.tablaForanea = tabla1;
        rel.campoForaneo = campo1;
        rel.tipoRelacion = "1:M";
    }
    else if (campo1EsPK && campo2EsPK) {
        // Ambos son PK: relación 1:1
        rel.tablaPrincipal = tabla1;
        rel.campoPrincipal = campo1;
        rel.tablaForanea = tabla2;
        rel.campoForaneo = campo2;
        rel.tipoRelacion = "1:1";
    }
    else {
        // Ninguno es PK: relación M:M (no validamos FK directamente)
        rel.tablaPrincipal = tabla1;
        rel.campoPrincipal = campo1;
        rel.tablaForanea = tabla2;
        rel.campoForaneo = campo2;
        rel.tipoRelacion = "M:M";
    }

    return rel;
}

Metadata ValidadorRelaciones::obtenerMetadata(const QString &nombreTabla)
{
    // Usar cache si está disponible
    if (cacheMetadatos.contains(nombreTabla)) {
        return cacheMetadatos[nombreTabla];
    }

    // Cargar desde archivo
    QDir dir(QDir::currentPath() + "/tables");
    QString rutaArchivo = dir.filePath(nombreTabla + ".meta");

    try {
        Metadata meta = Metadata::cargar(rutaArchivo);
        cacheMetadatos[nombreTabla] = meta; // Guardar en cache
        return meta;
    } catch (const std::exception &e) {
        qDebug() << "❌ Error cargando metadata para" << nombreTabla << ":" << e.what();
        return Metadata(); // Retornar metadata vacío
    }
}

bool ValidadorRelaciones::validarClaveForanea(const QString &tablaForanea,
                                              const QString &campoForaneo,
                                              const QVariant &valor)
{
    // Si el valor está vacío, considerarlo válido (permite NULL en FK)
    if (!valor.isValid() || valor.toString().trimmed().isEmpty()) {
        return true;
    }

    // Buscar la relación correspondiente
    for (const RelacionFK &rel : relaciones) {
        if (rel.tablaForanea == tablaForanea &&
            rel.campoForaneo == campoForaneo &&
            (rel.tipoRelacion == "1:M" || rel.tipoRelacion == "1:1")) {

            // Obtener metadata de la tabla principal
            Metadata metaPrincipal = obtenerMetadata(rel.tablaPrincipal);

            // Buscar el valor en los registros de la tabla principal
            for (const auto &registro : metaPrincipal.registros) {
                QVariant valorPrincipal = registro.value(rel.campoPrincipal);

                // Comparar valores (convertir a string para comparación)
                if (valorPrincipal.toString() == valor.toString()) {
                    qDebug() << "✅ FK válida:" << valor.toString() << "encontrado en"
                             << rel.tablaPrincipal << "." << rel.campoPrincipal;
                    return true; // ✅ Valor encontrado
                }
            }

            qDebug() << "❌ FK inválida:" << valor.toString() << "NO encontrado en"
                     << rel.tablaPrincipal << "." << rel.campoPrincipal;
            return false; // ❌ Valor no encontrado en tabla principal
        }
    }

    return true; // No hay relación FK para este campo, permitir cualquier valor
}

QStringList ValidadorRelaciones::obtenerValoresValidos(const QString &tablaForanea,
                                                       const QString &campoForaneo)
{
    QStringList valoresValidos;

    for (const RelacionFK &rel : relaciones) {
        if (rel.tablaForanea == tablaForanea &&
            rel.campoForaneo == campoForaneo &&
            (rel.tipoRelacion == "1:M" || rel.tipoRelacion == "1:1")) {

            Metadata metaPrincipal = obtenerMetadata(rel.tablaPrincipal);

            for (const auto &registro : metaPrincipal.registros) {
                QVariant valor = registro.value(rel.campoPrincipal);
                QString valorStr = valor.toString();
                if (!valorStr.isEmpty() && !valoresValidos.contains(valorStr)) {
                    valoresValidos.append(valorStr);
                }
            }
            break; // Solo necesitamos la primera relación que coincida
        }
    }

    valoresValidos.sort(); // Ordenar alfabéticamente
    return valoresValidos;
}

bool ValidadorRelaciones::esCampoClaveForanea(const QString &tabla, const QString &campo)
{
    for (const RelacionFK &rel : relaciones) {
        if (rel.tablaForanea == tabla &&
            rel.campoForaneo == campo &&
            (rel.tipoRelacion == "1:M" || rel.tipoRelacion == "1:1")) {
            return true;
        }
    }
    return false;
}

RelacionFK ValidadorRelaciones::obtenerRelacionFK(const QString &tablaForanea, const QString &campoForaneo)
{
    for (const RelacionFK &rel : relaciones) {
        if (rel.tablaForanea == tablaForanea &&
            rel.campoForaneo == campoForaneo &&
            (rel.tipoRelacion == "1:M" || rel.tipoRelacion == "1:1")) {
            return rel;
        }
    }
    return RelacionFK(); // Retornar relación vacía si no se encuentra
}

QStringList ValidadorRelaciones::validarEliminacion(const QString &tablaPrincipal,
                                                    const QString &campoPrincipal,
                                                    const QVariant &valor)
{
    QStringList dependencias;

    for (const RelacionFK &rel : relaciones) {
        if (rel.tablaPrincipal == tablaPrincipal &&
            rel.campoPrincipal == campoPrincipal &&
            (rel.tipoRelacion == "1:M" || rel.tipoRelacion == "1:1")) {

            Metadata metaForanea = obtenerMetadata(rel.tablaForanea);

            // Buscar registros que referencien este valor
            for (const auto &registro : metaForanea.registros) {
                QVariant valorForaneo = registro.value(rel.campoForaneo);
                if (valorForaneo.toString() == valor.toString()) {
                    dependencias.append(QString("%1.%2 = %3")
                                            .arg(rel.tablaForanea)
                                            .arg(rel.campoForaneo)
                                            .arg(valorForaneo.toString()));
                }
            }
        }
    }

    return dependencias;
}
