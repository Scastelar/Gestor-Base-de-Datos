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

bool ValidadorRelaciones::puedeCrearRelacionMM(const QString &tabla1, const QString &tabla2)
{
    Metadata meta1 = obtenerMetadata(tabla1);
    Metadata meta2 = obtenerMetadata(tabla2);

    bool tabla1TienePK = meta1.tienePK();
    bool tabla2TienePK = meta2.tienePK();

    // M:M solo permitida si NINGUNA tabla tiene PK
    bool permitida = !tabla1TienePK && !tabla2TienePK;

    qDebug() << "🔍 Verificando M:M entre" << tabla1 << "y" << tabla2
             << "- Tabla1 PK:" << tabla1TienePK
             << "- Tabla2 PK:" << tabla2TienePK
             << "- M:M permitida:" << permitida;

    return permitida;
}

RelacionFK ValidadorRelaciones::procesarRelacion(const QStringList &partes)
{
    QString tabla1 = partes[0];
    QString campo1 = partes[1];
    QString tabla2 = partes[2];
    QString campo2 = partes[3];

    Metadata meta1 = obtenerMetadata(tabla1);
    Metadata meta2 = obtenerMetadata(tabla2);

    bool tabla1TienePK = meta1.tienePK();
    bool tabla2TienePK = meta2.tienePK();
    bool campo1EsPK = false, campo2EsPK = false;

    for (const Campo &c : meta1.campos) {
        if (c.nombre == campo1 && c.esPK) { campo1EsPK = true; break; }
    }
    for (const Campo &c : meta2.campos) {
        if (c.nombre == campo2 && c.esPK) { campo2EsPK = true; break; }
    }

    RelacionFK rel;

    if (campo1EsPK && !campo2EsPK) {
        rel.tablaPrincipal = tabla1;
        rel.campoPrincipal = campo1;
        rel.tablaForanea = tabla2;
        rel.campoForaneo = campo2;
        rel.tipoRelacion = "1:M";
    }
    else if (campo2EsPK && !campo1EsPK) {
        rel.tablaPrincipal = tabla2;
        rel.campoPrincipal = campo2;
        rel.tablaForanea = tabla1;
        rel.campoForaneo = campo1;
        rel.tipoRelacion = "1:M";
    }
    else if (campo1EsPK && campo2EsPK) {
        rel.tablaPrincipal = tabla1;
        rel.campoPrincipal = campo1;
        rel.tablaForanea = tabla2;
        rel.campoForaneo = campo2;
        rel.tipoRelacion = "1:1";
    }
    else {
        // 🚨 M:M inválida si alguna tabla tiene PK
        if (tabla1TienePK || tabla2TienePK) {
            qDebug() << "❌ Relación M:M inválida: al menos una tabla tiene PK, no se guardará.";
            return RelacionFK(); // ← relación vacía, no se añade a relationships.dat
        }

        // ✅ M:M permitida
        rel.tablaPrincipal = tabla1;
        rel.campoPrincipal = campo1;
        rel.tablaForanea = tabla2;
        rel.campoForaneo = campo2;
        rel.tipoRelacion = "M:M";
        qDebug() << "✅ Relación M:M permitida entre" << tabla1 << "y" << tabla2;
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
        if (rel.tablaForanea == tablaForanea && rel.campoForaneo == campoForaneo) {

            if (rel.tipoRelacion == "M:M") {
                // Para relaciones M:M, no validamos FK tradicional
                // Los valores pueden ser cualquier cosa válida para el tipo de campo
                qDebug() << "🔗 Relación M:M detectada - validación FK omitida";
                return true;
            }

            if (rel.tipoRelacion == "1:M" || rel.tipoRelacion == "1:1") {
                // Obtener metadata de la tabla principal
                Metadata metaPrincipal = obtenerMetadata(rel.tablaPrincipal);

                // Buscar el valor en los registros de la tabla principal
                for (const auto &registro : metaPrincipal.registros) {
                    QVariant valorPrincipal = registro.value(rel.campoPrincipal);

                    if (valorPrincipal.toString() == valor.toString()) {
                        qDebug() << "✅ FK válida:" << valor.toString() << "encontrado en"
                                 << rel.tablaPrincipal << "." << rel.campoPrincipal;
                        return true;
                    }
                }

                qDebug() << "❌ FK inválida:" << valor.toString() << "NO encontrado en"
                         << rel.tablaPrincipal << "." << rel.campoPrincipal;
                return false;
            }
        }
    }

    return true; // No hay relación FK para este campo
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

QString ValidadorRelaciones::obtenerTipoRelacion(const QString &tabla1, const QString &campo1,
                                                 const QString &tabla2, const QString &campo2) const
{
    // Buscar relación directa
    for (const RelacionFK &rel : relaciones) {
        if ((rel.tablaPrincipal == tabla1 && rel.campoPrincipal == campo1 &&
             rel.tablaForanea == tabla2 && rel.campoForaneo == campo2) ||
            (rel.tablaPrincipal == tabla2 && rel.campoPrincipal == campo2 &&
             rel.tablaForanea == tabla1 && rel.campoForaneo == campo1)) {
            return rel.tipoRelacion;
        }
    }
    return ""; // No encontrado
}

void ValidadorRelaciones::debugRelaciones(const QString &nombreTabla) const
{
    qDebug() << "🔍 DEBUG RELACIONES" << (nombreTabla.isEmpty() ? "(TODAS)" : "PARA " + nombreTabla);
    qDebug() << "📊 Total de relaciones cargadas:" << relaciones.size();

    for (int i = 0; i < relaciones.size(); i++) {
        const RelacionFK &rel = relaciones[i];

        if (nombreTabla.isEmpty() ||
            rel.tablaPrincipal == nombreTabla ||
            rel.tablaForanea == nombreTabla) {

            qDebug() << QString("  %1: %2.%3 (%4) -> %5.%6 (%7) [%8]")
            .arg(i + 1)
                .arg(rel.tablaPrincipal).arg(rel.campoPrincipal)
                .arg("PK")
                .arg(rel.tablaForanea).arg(rel.campoForaneo)
                .arg("FK")
                .arg(rel.tipoRelacion);
        }
    }
}
