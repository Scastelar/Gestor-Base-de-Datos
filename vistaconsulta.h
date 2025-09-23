#ifndef VISTACONSULTA_H
#define VISTACONSULTA_H

#include <QWidget>
#include <QTableWidget>
#include <QVBoxLayout>
#include "metadata.h"
#include <QDebug>

//BTree
template<typename K, typename V>
class ArbolB {
    struct Nodo {
        bool hoja;
        QVector<K> claves;
        QVector<V> valores;
        QVector<Nodo*> hijos;
        Nodo(bool esHoja) : hoja(esHoja) {}
    };
    Nodo* raiz;
    int t; // grado mínimo

public:
    ArbolB(int gradoMinimo = 2) : raiz(new Nodo(true)), t(gradoMinimo) {}

    void insertar(const K& clave, const V& valor) {
        raiz->claves.append(clave);
        raiz->valores.append(valor);
    }

    QVector<V> buscar(const K& clave) {
        QVector<V> resultado;
        for (int i = 0; i < raiz->claves.size(); i++) {
            if (raiz->claves[i] == clave) resultado.append(raiz->valores[i]);
        }
        return resultado;
    }

    QVector<V> recorrer() {
        return raiz->valores;
    }
};


// -----------------------------
//  VistaConsulta
// -----------------------------
class VistaConsulta : public QWidget
{
    Q_OBJECT
public:
    explicit VistaConsulta(QWidget *parent = nullptr);

    void mostrarConsulta(const QString &sqlLike,
                         const QVector<Metadata> &tablas);

private:
    QTableWidget *tablaResultados;

    void aplicarConsulta(const QString &sqlLike,
                         const QVector<Metadata> &tablas);
};

#endif // VISTACONSULTA_H
