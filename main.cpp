#include <QtGui>
#include "graphicsview.h"
#include "graphicsscene.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    GraphicsScene scene;
    GraphicsView w(&scene);
    w.show();
    return a.exec();
}
