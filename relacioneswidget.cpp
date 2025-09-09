#include "relacioneswidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolBar>
#include <QToolButton>
#include <QFrame>
#include <QScrollArea>
#include <QLabel>
#include <QDir>
#include <QPushButton>
#include <QGroupBox>
#include <QCloseEvent>

RelacionesWidget::RelacionesWidget(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #ffffff; color: #000000;");
    crearToolbar();
    crearLayoutPrincipal();
}

void RelacionesWidget::crearToolbar()
{
    QToolBar *toolbar = new QToolBar(this);
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(16, 16));

    // Botón Cerrar
    QToolButton *btnCerrar = new QToolButton();
    btnCerrar->setText("Cerrar");
    connect(btnCerrar, &QToolButton::clicked, this, &QWidget::close);

    toolbar->addWidget(btnCerrar);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(toolbar);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);
}

void RelacionesWidget::crearLayoutPrincipal()
{
    QVBoxLayout *mainLayout = qobject_cast<QVBoxLayout*>(layout());

    QHBoxLayout *contentLayout = new QHBoxLayout();

    // Panel izquierdo: lista de tablas
    QFrame *panelTablas = new QFrame();
    QVBoxLayout *panelLayout = new QVBoxLayout(panelTablas);

    QGroupBox *grupoTablas = new QGroupBox("Tablas Disponibles");
    QVBoxLayout *tablasLayout = new QVBoxLayout(grupoTablas);

    listaTablas = new QListWidget();
    tablasLayout->addWidget(listaTablas);

    QPushButton *btnAgregar = new QPushButton("Agregar");
    tablasLayout->addWidget(btnAgregar);

    panelLayout->addWidget(grupoTablas);
    panelTablas->setFixedWidth(250);

    // Área central scrollable para las cards
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);

    QWidget *scrollContent = new QWidget();
    cardsLayout = new QVBoxLayout(scrollContent);
    cardsLayout->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    cardsLayout->setContentsMargins(15, 15, 15, 15);
    cardsLayout->setSpacing(15);

    scrollArea->setWidget(scrollContent);

    contentLayout->addWidget(panelTablas);
    contentLayout->addWidget(scrollArea, 1);

    mainLayout->addLayout(contentLayout);

    // 🔹 Llenar la lista de tablas reales
    cargarListaTablas();

    // 🔹 Acción de Agregar
    connect(btnAgregar, &QPushButton::clicked, this, [this]() {
        QListWidgetItem *item = listaTablas->currentItem();
        if (!item) return;

        QString tablaSeleccionada = item->text();
        QDir dir(QDir::currentPath() + "/tables");
        Metadata meta = Metadata::cargar(dir.filePath(tablaSeleccionada + ".meta"));
        crearCardTabla(meta);
    });
}

void RelacionesWidget::cargarListaTablas()
{
    QDir dir(QDir::currentPath() + "/tables");
    QStringList archivosMeta = dir.entryList(QStringList() << "*.meta", QDir::Files);

    for (const QString &fileName : archivosMeta) {
        QString nombreTabla = fileName;
        nombreTabla.chop(5); // quitar ".meta"
        listaTablas->addItem(nombreTabla);
    }
}

void RelacionesWidget::crearCardTabla(const Metadata &meta)
{
    QFrame *card = new QFrame();
    card->setStyleSheet("QFrame { background: #fff; border: 2px solid #2b579a; border-radius: 5px; }");

    QVBoxLayout *cardLayout = new QVBoxLayout(card);

    QLabel *titulo = new QLabel(meta.nombreTabla);
    titulo->setStyleSheet("font-weight: bold; font-size: 12px; background: #2b579a; color: white; padding: 6px;");
    titulo->setAlignment(Qt::AlignCenter);

    cardLayout->addWidget(titulo);

    for (const Campo &campo : meta.campos) {
        QLabel *labelCampo = new QLabel("• " + campo.nombre + " (" + campo.tipo + ")");
        labelCampo->setStyleSheet("font-size: 11px;");
        cardLayout->addWidget(labelCampo);
    }

    cardsLayout->addWidget(card);
}

bool RelacionesWidget::eventFilter(QObject *obj, QEvent *event)
{
    return QWidget::eventFilter(obj, event);
}

void RelacionesWidget::closeEvent(QCloseEvent *event)
{
    emit cerrada();
    event->accept();
}




