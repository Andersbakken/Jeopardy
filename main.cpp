#include <QtGui>
#include "graphicsview.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    GraphicsView w;
    w.show();
    return a.exec();
}
