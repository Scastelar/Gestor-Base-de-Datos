#ifndef AVAILDATOS_H
#define AVAILDATOS_H

#include <QVector>
#include <QMap>
#include <QString>
#include <QVariant>
#include <QDebug>
#include "metadata.h"

struct BloqueDisponible {
    int inicio;
    int tamaño;
    bool ocupado;
    BloqueDisponible(int ini, int tam) : inicio(ini), tamaño(tam), ocupado(false) {}
};

class AvailDatos {
private:
    QVector<BloqueDisponible> bloques;
    int memoriaTotal;
    QVector<Campo> campos;
    QVector<QMap<QString, QVariant>> registros;

public:
    AvailDatos(int tamMemoria = 1000) {
        inicializar(tamMemoria);
    }

    void inicializar(int tamMemoria) {
        memoriaTotal = tamMemoria;
        bloques.clear();
        bloques.append(BloqueDisponible(0, tamMemoria));
    }

    //Operaciones con campos
    bool insertarCampo(const Campo& nuevoCampo, int tamaño) {
        int dir = asignar(tamaño);
        if (dir != -1) {
            campos.append(nuevoCampo);
            qDebug() << "Campo" << nuevoCampo.nombre << "insertado en dir" << dir;
            return true;
        }
        return false;
    }

    bool eliminarCampo(const QString& nombreCampo) {
        for (int i = 0; i < campos.size(); i++) {
            if (campos[i].nombre == nombreCampo) {

                if (!bloques.isEmpty()) {
                    liberar(bloques[0].inicio);
                }
                campos.remove(i);
                qDebug() << "Campo" << nombreCampo << "eliminado";
                return true;
            }
        }
        return false;
    }

    bool modificarCampo(const QString& nombreCampo, const Campo& campoModificado, int nuevoTam) {
        for (int i = 0; i < campos.size(); i++) {
            if (campos[i].nombre == nombreCampo) {
                campos[i] = campoModificado;
                modificar(bloques[0].inicio, nuevoTam);
                qDebug() << "Campo" << nombreCampo << "modificado";
                return true;
            }
        }
        return false;
    }

    QVector<Campo> listarCampos() const {
        return campos;
    }

    //Operaciones con registros
    void insertarRegistro(const QMap<QString, QVariant>& registro, int tamaño) {
        int dir = asignar(tamaño);
        if (dir != -1) {
            registros.append(registro);
            qDebug() << "Registro insertado en dir" << dir;
        }
    }

    bool eliminarRegistro(int indice) {
        if (indice >= 0 && indice < registros.size()) {
            liberar(bloques[0].inicio); // simulación
            registros.remove(indice);
            qDebug() << "Registro eliminado en índice" << indice;
            return true;
        }
        return false;
    }

    QVector<QMap<QString, QVariant>> listarRegistros() const {
        return registros;
    }

    int asignar(int tamañoSolicitado) {
        for (int i = 0; i < bloques.size(); i++) {
            if (!bloques[i].ocupado && bloques[i].tamaño >= tamañoSolicitado) {
                int direccion = bloques[i].inicio;
                if (bloques[i].tamaño > tamañoSolicitado) {
                    BloqueDisponible nuevo(bloques[i].inicio + tamañoSolicitado,
                                           bloques[i].tamaño - tamañoSolicitado);
                    bloques.insert(i + 1, nuevo);
                }
                bloques[i].tamaño = tamañoSolicitado;
                bloques[i].ocupado = true;
                return direccion;
            }
        }
        return -1;
    }

    bool liberar(int direccion) {
        for (int i = 0; i < bloques.size(); i++) {
            if (bloques[i].inicio == direccion && bloques[i].ocupado) {
                bloques[i].ocupado = false;
                compactar();
                return true;
            }
        }
        return false;
    }

    bool modificar(int direccion, int nuevoTam) {
        if (liberar(direccion)) {
            return asignar(nuevoTam) != -1;
        }
        return false;
    }

    void compactar() {
        for (int i = 0; i < bloques.size() - 1; i++) {
            if (!bloques[i].ocupado && !bloques[i + 1].ocupado &&
                bloques[i].inicio + bloques[i].tamaño == bloques[i + 1].inicio) {
                bloques[i].tamaño += bloques[i + 1].tamaño;
                bloques.remove(i + 1);
                i--;
            }
        }
    }

};

#endif // AVAILDATOS_H
