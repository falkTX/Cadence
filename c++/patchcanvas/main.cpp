#include <QtGui/QApplication>
#include "canvastestapp.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CanvasTestApp w;
    w.show();

    return a.exec();
}
