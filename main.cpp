#include <QtGui>
#include "graphicsview.h"
#include "graphicsscene.h"

int main(int argc, char **argv)
{
    QApplication a(argc, argv);
    GraphicsScene scene;
    if (!scene.load("questions.txt"))
        return 1;
    GraphicsView w(&scene);
    w.show();
    return a.exec();
}
