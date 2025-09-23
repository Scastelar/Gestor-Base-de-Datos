#ifndef VALIDADORRELACIONES_H
#define VALIDADORRELACIONES_H

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QStringList>
#include "metadata.h"

struct RelacionFK {
    QString tablaPrincipal;     // Tabla "1" (donde está la PK)
    QString campoPrincipal;     // Campo PK
    QString tablaForanea;       // Tabla "M" (donde está la FK)
    QString campoForaneo;       // Campo FK
    QString tipoRelacion;       // "1:M", "1:1", "M:M"

    bool esValida() const {
        return !tablaPrincipal.isEmpty() && !campoPrincipal.isEmpty() &&
               !tablaForanea.isEmpty() && !campoForaneo.isEmpty();
    }
};

class ValidadorRelaciones : public QObject
{
    Q_OBJECT

public:
    explicit ValidadorRelaciones(QObject *parent = nullptr);

    // Cargar relaciones desde archivo
    void cargarRelaciones(const QString &archivoRelaciones = "relationships.dat");

    // Validar si un valor existe en la tabla principal (PK)
    bool validarClaveForanea(const QString &tablaForanea,
                             const QString &campoForaneo,
                             const QVariant &valor);

    // Obtener todos los valores válidos para una clave foránea
    QStringList obtenerValoresValidos(const QString &tablaForanea,
                                      const QString &campoForaneo);

    bool puedeCrearRelacionMM(const QString &tabla1, const QString &tabla2);

    // Verificar si un campo es clave foránea
    bool esCampoClaveForanea(const QString &tabla, const QString &campo);

    // Obtener información de la relación FK para un campo
    RelacionFK obtenerRelacionFK(const QString &tablaForanea, const QString &campoForaneo);

    // Validar integridad antes de eliminar un registro de tabla principal
    QStringList validarEliminacion(const QString &tablaPrincipal,
                                   const QString &campoPrincipal,
                                   const QVariant &valor);

    // NUEVO método para debug completo de relaciones
    void debugRelaciones(const QString &nombreTabla = "") const;

    // MEJORAR método para verificar tipo de relación más específico
    QString obtenerTipoRelacion(const QString &tabla1, const QString &campo1,
                                const QString &tabla2, const QString &campo2) const;

private:
    QList<RelacionFK> relaciones;
    QMap<QString, Metadata> cacheMetadatos;

    // Cargar metadata de una tabla (con cache)
    Metadata obtenerMetadata(const QString &nombreTabla);

    // Determinar qué tabla es la principal (tiene PK) en una relación 1:M
    RelacionFK procesarRelacion(const QStringList &partes);
};

#endif // VALIDADORRELACIONES_H
