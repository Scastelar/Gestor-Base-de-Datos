#include "basedatoswindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{

    QApplication a(argc, argv);
    BaseDatosWindow w;
    w.show();
    return a.exec();

}
